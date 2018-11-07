#ifndef SIGNALING_SERVER_DATA_SOCKET_H_
#define SIGNALING_SERVER_DATA_SOCKET_H_

#ifdef _WIN32
#include <winsock2.h>
typedef int socklen_t;
typedef SOCKET NativeSocket;
#else //linux
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#define closesocket close
typedef int NativeSocket;

#ifndef SOCKET_ERROR
#define SOCKET_ERROR (-1)
#endif

#ifndef INVALID_SOCKET
#define INVALID_SOCKET static_cast<NativeSocket>(-1)
#endif //linux end

#endif
#include <string>

class SocketBase {
 public:
  SocketBase(): socket_(INVALID_SOCKET) {}
  explicit SocketBase(NativeSocket socket): socket_(socket) {}
  ~SocketBase() { Close(); }

  NativeSocket socket() const { return socket_; }
  bool valid() const { return socket_ != INVALID_SOCKET; }

  bool Create();
  void Close();

 protected:
  NativeSocket socket_;
};

//Represents an HTTP server socket.
class DataSocket : public SocketBase {
 public:
  enum RequestMethod {
    INVALID,
    GET,
    POST,
    OPTIONS
  };

  explicit DataSocket(NativeSocket socket)
      : SocketBase(socket), method_(INVALID), content_length_(0) {}

  ~DataSocket() {}
  
  static const char kCrossOriginAllowHeaders[];

  bool headers_received() const { return method_ != INVALID; }

  RequestMethod method() const { return method_; }

  const std::string& request_path() const { return request_path_; }
  std::string request_arguments() const;
  
  const std::string& data() const { return data_; }

  const std::string& content_type() const { return content_type_; }

  size_t content_length() const { return content_length_; }

   bool request_received() const {
    return headers_received() && (method_ != POST || data_received());
  }

    bool data_received() const {
    return method_ != POST || data_.length() >= content_length_;
  }

  // Checks if the request path (minus arguments) matches a given path.
  bool PathEquals(const char* path) const;

  //Called when we have received some data from clients.
  //Returns false if an error occurred.
  bool OnDataAvailable(bool* close_socket);

  //Send a raw buffer of bytes.
  bool Send(const std::string& data) const;

  // Clears all held state and prepares the socket for receiving a new request.
  void Clear();

 protected:
  // A fairly relaxed HTTP header parser.  Parses the method, path and
  // content length (POST only) of a request.
  // Returns true if a valid request was received and no errors occurred.
  bool ParseHeaders();

  //Figures out whether the request is a GET or POST and what path is
  //being requested.
  bool ParseMethodAndPath(const char* begin, size_t len);

  //Determines the length of the body and it's mine type.
  bool ParseContentLengthAndType(const char* headers, size_t length);

 protected:
  RequestMethod method_;
  size_t content_length_;
  std::string content_type_;
  std::string request_path_;
  std::string request_headers_;
  std::string data_;
};

// The server socket.  Accepts connections and generates DataSocket instances
// for each new connection.
class ListeningSocket : public SocketBase {
 public:
  ListeningSocket() {}

  bool Listen(unsigned short port);
  DataSocket* Accept() const;
};

#endif