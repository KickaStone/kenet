add_gtest(Eventloop_test 
    TYPE unit
    LIBS netcore
    SRCS Eventloop_test.cpp
    LABELS EventLoop
)

add_gtest(Poller_test 
    TYPE unit
    LIBS netcore
    SRCS Poller_test.cpp
    LABELS Poller
)

add_gtest(InetAddress_test 
    TYPE unit
    LIBS netcore
    SRCS InetAddress_test.cpp
    LABELS InetAddress
)