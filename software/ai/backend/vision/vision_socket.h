#ifndef AI_BACKEND_SSL_VISION_VISION_SOCKET_H
#define AI_BACKEND_SSL_VISION_VISION_SOCKET_H

#include <glibmm/iochannel.h>
#include <sigc++/signal.h>
#include <string>
#include "util/fd.h"
#include "util/noncopyable.h"

class SSL_WrapperPacket;

namespace AI
{
namespace BE
{
namespace Vision
{
class VisionSocket final : public NonCopyable
{
   public:
    sigc::signal<void, const SSL_WrapperPacket &> signal_vision_data;

    explicit VisionSocket(int multicast_interface, const std::string &port);
    ~VisionSocket();

   private:
    FileDescriptor sock;
    sigc::connection conn;
    bool receive_packet(Glib::IOCondition);
};
}
}
}

#endif
