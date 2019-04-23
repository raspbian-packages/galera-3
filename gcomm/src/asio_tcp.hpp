/*
 * Copyright (C) 2010-2017 Codership Oy <info@codership.com>
 */

#ifndef GCOMM_ASIO_TCP_HPP
#define GCOMM_ASIO_TCP_HPP

#include "socket.hpp"
#include "asio_protonet.hpp"

#include "gu_array.hpp"
#include "gu_shared_ptr.hpp"

#include <boost/bind.hpp>
#include <vector>
#include <deque>

//
// Boost and std:: enable_shared_from_this<> does not have virtual destructor,
// therefore need to ignore -Weffc++ and -Wnon-virtual-dtor
//
#if defined(__GNUG__)
# if (__GNUC__ == 4 && __GNUC_MINOR__ >= 6) || (__GNUC__ > 4)
#  pragma GCC diagnostic push
# endif // (__GNUC__ == 4 && __GNUC_MINOR__ >= 6) || (__GNUC__ > 4)
# pragma GCC diagnostic ignored "-Weffc++"
# pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#endif

namespace gcomm
{
    class AsioTcpSocket;
    class AsioTcpAcceptor;
    class AsioPostForSendHandler;
}

// TCP Socket implementation

class gcomm::AsioTcpSocket :
    public gcomm::Socket,
    public gu::enable_shared_from_this<AsioTcpSocket>::type
{
public:
    AsioTcpSocket(AsioProtonet& net, const gu::URI& uri);
    ~AsioTcpSocket();
    void failed_handler(const asio::error_code& ec, const std::string& func, int line);
    void handshake_handler(const asio::error_code& ec);
    void connect_handler(const asio::error_code& ec);
    void connect(const gu::URI& uri);
    void close();
    void write_handler(const asio::error_code& ec,
                       size_t bytes_transferred);
    void set_option(const std::string& key, const std::string& val);
    int send(const Datagram& dg);
    size_t read_completion_condition(
        const asio::error_code& ec,
        const size_t bytes_transferred);
    void read_handler(const asio::error_code& ec,
                      const size_t bytes_transferred);
    void async_receive();
    size_t mtu() const;
    std::string local_addr() const;
    std::string remote_addr() const;
    State state() const { return state_; }
    SocketId id() const { return &socket_; }
private:
    friend class gcomm::AsioTcpAcceptor;
    friend class gcomm::AsioPostForSendHandler;

    AsioTcpSocket(const AsioTcpSocket&);
    void operator=(const AsioTcpSocket&);

    void set_socket_options();
    void read_one(gu::array<asio::mutable_buffer, 1>::type& mbs);
    void write_one(const gu::array<asio::const_buffer, 2>::type& cbs);
    void close_socket();

    // call to assign local/remote addresses at the point where it
    // is known that underlying socket is live
    void assign_local_addr();
    void assign_remote_addr();

    // returns real socket to use
    typedef asio::basic_socket<asio::ip::tcp,
                               asio::stream_socket_service<asio::ip::tcp> >
    basic_socket_t;
    basic_socket_t&
    socket() { return (ssl_socket_ ? ssl_socket_->lowest_layer() : socket_); }

    AsioProtonet&                             net_;
    asio::ip::tcp::socket                     socket_;
    asio::ssl::stream<asio::ip::tcp::socket>* ssl_socket_;
    std::deque<Datagram>                      send_q_;
    std::vector<gu::byte_t>                   recv_buf_;
    size_t                                    recv_offset_;
    State                                     state_;
    // Querying addresses from failed socket does not work,
    // so need to maintain copy for diagnostics logging
    std::string                               local_addr_;
    std::string                               remote_addr_;

    template <typename T> unsigned long long
    check_socket_option(const std::string& key, unsigned long long val)
    {
        T option;
        socket().get_option(option);
        if (val != static_cast<unsigned long long>(option.value()))
        {
            log_info << "Setting '" << key << "' to " << val
                     << " failed. Resulting value is " << option.value();
        }
        return option.value();
    }
};

class gcomm::AsioTcpAcceptor : public gcomm::Acceptor
{
public:

    AsioTcpAcceptor(AsioProtonet& net, const gu::URI& uri);
    ~AsioTcpAcceptor();
    void listen(const gu::URI& uri);
    std::string listen_addr() const;
    void close();
    SocketPtr accept();

    State state() const
    {
        gu_throw_fatal << "TODO:";
    }

    SocketId id() const { return &acceptor_; }

private:
    void accept_handler(
        SocketPtr socket,
        const asio::error_code& error);

    AsioProtonet& net_;
    asio::ip::tcp::acceptor acceptor_;
    SocketPtr accepted_socket_;
};

#if defined(__GNUG__)
# if (__GNUC__ == 4 && __GNUC_MINOR__ >= 6) || (__GNUC__ > 4)
#  pragma GCC diagnostic pop
# endif // (__GNUC__ == 4 && __GNUC_MINOR__ >= 6) || (__GNUC__ > 4)
#endif

#endif // GCOMM_ASIO_TCP_HPP
