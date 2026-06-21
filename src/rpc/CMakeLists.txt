add_library(rpc_lib 
    proto/rpcheader.pb.cc
    mprpccontroller.cpp
    mprpcchannel.cpp
    rpcprovider.cpp

    )
target_link_libraries(rpc_lib protobuf)