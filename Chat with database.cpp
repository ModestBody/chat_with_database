// Chat with database.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <boost/asio.hpp>
#include "network.cpp"
#include "database.cpp"

using boost::asio::ip::tcp;

int main(int argc, char* argv[]) {
    try {
        if (argc < 5) {
            std::cerr << "Usage: " << argv[0] << " <mode> <dsn> <user> <password> [host] [port]" << std::endl;
            return 1;
        }

        std::string dsn = argv[2];
        std::string user = argv[3];
        std::string password = argv[4];
        ChatDatabase db(dsn, user, password);

        boost::asio::io_context io_context;
        if (std::string(argv[1]) == "server") {
            if (argc != 6) {
                std::cerr << "Usage: " << argv[0] << " server <dsn> <user> <password> <port>" << std::endl;
                return 1;
            }

            tcp::endpoint endpoint(tcp::v4(), std::atoi(argv[5]));
            ChatServer server(io_context, endpoint);
            io_context.run();
        }
        else if (std::string(argv[1]) == "client") {
            if (argc != 7) {
                std::cerr << "Usage: " << argv[0] << " client <dsn> <user> <password> <host> <port>" << std::endl;
                return 1;
            }

            tcp::resolver resolver(io_context);
            auto endpoints = resolver.resolve(argv[5], argv[6]);
            ChatClient client(io_context, endpoints);
            std::thread t([&io_context]() { io_context.run(); });

            std::string line;
            while (std::getline(std::cin, line)) {
                db.insertMessage("Client", line);
                client.write(line);
            }

            client.close();
            t.join();
        }
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
