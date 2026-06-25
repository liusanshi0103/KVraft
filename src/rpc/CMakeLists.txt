add_library(rpc_lib 
    proto/rpcheader.pb.cc
    mprpccontroller.cpp
    mprpcchannel.cpp
    rpcprovider.cpp
    mprpcconfig.cpp

    )
target_link_libraries(rpc_lib protobuf)