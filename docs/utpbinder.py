"""
  telehash binder implmeneted in C++.
"""


def init(port, channel__handler_factory, broadcast_handler):
    """
    init.

    :param int port: port number to be listened packets. if 0, port number is.
    :param Method channel_handler_factory: method for creating ChannelHandler.
                                           called when requested openning.
                                           channel.
    :param method broadcast_handler: method for handling broadcasted message.
    """
    pass


def open_channel(cobj, location, name, handler):
    """
    open a channel associated a channel name.

    :param Object cobj: pointer of StorjTelehash instnace returned by init()
    :param str location: json str where you want to open a channel.
    :param str name: channel name that you want to open .
    :param Method handler: method for handling channel packets.
                           called every time when receiving packets.
    """
    pass


def add_broadcaster(cobj, location, add):
    """
    send a broadcast request to broadcaster.
    After calling this method, broadcast messages will be send continually.

    :param Object cobj: pointer of StorjTelehash instnace returned by init()
    :param str location: json str where you want to request a broadcast.
    :param  int add: if 0, request to not to  broadcast. request to broaadcast
                     if others.
    """
    pass


def broadcast(cobj, location, message):
    """
    broadcast a message.

    :param Object cobj: pointer of StorjTelehash instnace returned by init()
    :param str location: json str where you want to send a broadcast.
    :param str message: broadcast message.
    """
    pass


def start(cobj):
    """
    star to receive netowrk packets in the current thread..

    :param Object cobj: pointer of StorjTelehash instnace returned by init()
    """
    pass


def set_stopflag(cobj, stop):
    """
    set stopFlag that stop/continue to receive network packets loop..

    :param Object cobj: pointer of StorjTelehash instnace returned by init()
    :param int flag: 1 if to stop. others if to run.
    """
    pass


def finalize(cobj):
    """
    destructor.
    delete factory, StorjTelehash instnace, and broadcasthandler.

    :param Object cobj: pointer of StorjTelehash instnace returned by init()
    """
    pass


def set_gc(cobj, use):
    """
    set  GC to stop or not. test use only. don't use it.

    :param Object cobj: pointer of StorjTelehash instnace returned by init()
    :param int use:  0 if you do not want to use. others if not.
     """
    pass


def gcollect(cobj):
    """
    run force garbage collect.

    :param Object cobj: pointer of StorjTelehash instnace returned by init()
     """
    pass


def get_my_location(cobj):
    """
     return my location information. format is:

     {"keys":{"1a":"al45izsjxe2sikv7mc6jpnwywybbkqvsou"},
     paths":[{"type":"udp4","ip":"127.0.0.1","port":1234}]

     :param Object cobj: pointer of StorjTelehash instnace returned by init()
     :return: location info.
     """
    pass
