#include <iostream>
#include <sstream>

#include "flatbuffers/flatbuffers.h"
#include "flatbuffers/flatbuffer_builder.h"
#include "fbs/ClientHello_generated.h"
#include "flatbuffers/minireflect.h"
int main() {
    std::vector<unsigned char> bufferSaved = {};
    {
        flatbuffers::FlatBufferBuilder builder;
        flatbuffers::Offset<Network::Packets::ClientHello> clientHello = Network::Packets::CreateClientHello(builder, 1);
        builder.Finish(clientHello);

        auto buffer = builder.GetBufferSpan();
        bufferSaved = std::vector(buffer.begin(), buffer.end());

        auto h = Network::Packets::GetClientHello(buffer.data());
        // std::cout <<  flatbuffers::FlatBufferToString(buffer.data(), Network::Packets::ClientHelloTypeTable()) << std::endl;

        builder.Clear();
    }

    // flatbuffers::Verifier verifier(bufferSaved.data(), bufferSaved.size());
    // if (!VerifyTestFBSTableBuffer(verifier)) {
    //     std::cout << "TestFBSTable verification failed." << std::endl;
    //     return -1;
    // }
    //
    // auto test = GetTestFBSTable(bufferSaved.data());
    // std::cout << test->test1() << std::endl;
    // std::cout << test->test2() << std::endl;
    // std::cout << test->test3() << std::endl;

    return 0;
}
