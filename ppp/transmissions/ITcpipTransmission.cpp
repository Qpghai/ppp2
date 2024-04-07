#include <ppp/transmissions/ITcpipTransmission.h>
#include <ppp/net/Socket.h>
#include <ppp/net/Ipep.h>
#include <ppp/net/IPEndPoint.h>
#include <ppp/threading/Executors.h>
#include <ppp/coroutines/asio/asio.h>
#include <ppp/coroutines/YieldContext.h>

using ppp::net::Socket;
using ppp::net::IPEndPoint;

namespace ppp {
    namespace transmissions {
        ITcpipTransmission::ITcpipTransmission(
            const ContextPtr&                                       context, 
            const StrandPtr&                                        strand,
            const std::shared_ptr<boost::asio::ip::tcp::socket>&    socket, 
            const AppConfigurationPtr&                              configuration) noexcept 
            : ITransmission(context, strand, configuration)
            , disposed_(false)
            , socket_(socket) {
            boost::system::error_code ec;
            remoteEP_ = ppp::net::Ipep::V6ToV4(socket->remote_endpoint(ec));
        }

        ITcpipTransmission::~ITcpipTransmission() noexcept {
            Finalize();
        }
 
        void ITcpipTransmission::Finalize() noexcept {
            if (std::shared_ptr<boost::asio::ip::tcp::socket> socket = std::move(socket_); socket) {
                socket_.reset();
                Socket::Closesocket(socket);
            }

            disposed_ = true;
        }

        void ITcpipTransmission::Dispose() noexcept {
            auto self = shared_from_this();
            ppp::threading::Executors::Post(GetContext(), GetStrand(),
                [self, this]() noexcept {
                    Finalize();
                });
            ITransmission::Dispose();
        }

        boost::asio::ip::tcp::endpoint ITcpipTransmission::GetRemoteEndPoint() noexcept {
            return remoteEP_;
        }

        std::shared_ptr<Byte> ITcpipTransmission::DoReadBytes(YieldContext& y, int length) noexcept {
            if (disposed_) {
                return NULL;
            }

            auto self = shared_from_this();
            return ITransmissionQoS::DoReadBytes(y, length, self, *this, this->QoS);
        }

        bool ITcpipTransmission::ShiftToScheduler() noexcept {
            std::shared_ptr<boost::asio::ip::tcp::socket> socket = socket_;
            if (!socket || !socket->is_open()) {
                return false;
            }

            if (disposed_) {
                return false;
            }

            std::shared_ptr<boost::asio::ip::tcp::socket> socket_new;
            ContextPtr scheduler;
            StrandPtr strand;

            bool ok = ppp::threading::Executors::ShiftToScheduler(*socket, socket_new, scheduler, strand);
            if (ok) {
                socket_ = socket_new;
                GetStrand() = strand;
                GetContext() = scheduler;
            }

            return ok;
        }

        std::shared_ptr<Byte> ITcpipTransmission::ReadBytes(YieldContext& y, int length) noexcept {
            std::shared_ptr<boost::asio::ip::tcp::socket> socket = socket_;
            if (!socket || !socket->is_open()) {
                return NULL;
            }

            if (disposed_) {
                return NULL;
            }

            if (length < 1) {
                return NULL;
            }

            std::shared_ptr<BufferswapAllocator> allocator = this->BufferAllocator;
            std::shared_ptr<Byte> packet = BufferswapAllocator::MakeByteArray(allocator, length);
            if (NULL == packet) {
                return NULL;
            }

            bool ok = ppp::coroutines::asio::async_read(*socket, boost::asio::buffer(packet.get(), length), y);
            if (!ok) {
                Dispose();
                return NULL;
            }

            std::shared_ptr<ITransmissionStatistics> statistics = this->Statistics;
            if (statistics) {
                statistics->AddIncomingTraffic(length);
            }

            return packet;
        }

        bool ITcpipTransmission::DoWriteBytes(std::shared_ptr<Byte> packet, int offset, int packet_length, const AsynchronousWriteBytesCallback& cb) noexcept {
            std::shared_ptr<boost::asio::ip::tcp::socket> socket = socket_;
            if (!socket || !socket->is_open()) {
                return false;
            }

            if (disposed_) {
                return false;
            }

            std::shared_ptr<IAsynchronousWriteIoQueue> self = shared_from_this();
            auto complete_do_write_bytes_async_callback = [self, this, socket, packet, offset, packet_length, cb]() noexcept {
                boost::asio::async_write(*socket, boost::asio::buffer((Byte*)packet.get() + offset, packet_length),
                    [self, this, packet, packet_length, cb](const boost::system::error_code& ec, std::size_t sz) noexcept {
                        bool ok = ec == boost::system::errc::success;
                        if (ok) {
                            std::shared_ptr<ITransmissionStatistics> statistics = this->Statistics;
                            if (statistics) {
                                statistics->AddOutgoingTraffic(packet_length);
                            }
                        }
                        else {
                            Dispose();
                        }

                        if (cb) {
                            cb(ok);
                        }
                    });
                };

            return ppp::threading::Executors::Dispatch(GetContext(), GetStrand(), complete_do_write_bytes_async_callback);
        }
    }
}