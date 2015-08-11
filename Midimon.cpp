#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <cstdlib>
#if defined _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif
#include <stdint.h>
#include "RtMidi.h"

static void show_usage(std::string name)
{
    std::cerr << "Usage: " << name << " [-l] [-p <portnumber>]\n"
              << "Options:\n"
              << "\t-h,--help\t\t\tShow this help message\n"
              << "\t-l,--list-ports \t\tList all available ports\n"
              << "\t-p <port>,--port <portnumber>\tlisten on specified port\n"
              << std::endl;
}

char *progName;

void bye(int retval)
{
    show_usage(progName);
    exit(retval);
}

void mycallback(double stamp, std::vector<unsigned char> *message, void *userData)
{
    int nBytes = message->size();
    if (nBytes > 0) {
        std::cout << std::setfill(' ') << std::setw(8) << stamp << ": ";
        for (int i = 0; i < nBytes; i++)
            std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)message->at(i) << " ";

        if (nBytes == 3) {
            uint8_t type = message->at(0) & 0xf0;
            uint8_t channel = message->at(0) & 0xf;

            switch (type) {
            case 0x90:
                std::cout << std::dec << " note on        (channel " << std::setw(2) << (int)channel << "): pitch " << std::setw(3) << (int)message->at(1) << ", velocity " << (int)message->at(2);
                break;
            case 0x80:
                std::cout << std::dec << " note off       (channel " << std::setw(2) << (int)channel << "): pitch " << std::setw(3) << (int)message->at(1) << ", velocity " << (int)message->at(2);
                break;
            case 0xb0:
                std::cout << std::dec << " control change (channel " << std::setw(2) << (int)channel << "): controller " << std::setw(3) << (int)message->at(1) << ", value " << (int)message->at(2);
                break;
            case 0xe0:
                std::cout << std::dec << " pitch bender   (channel " << std::setw(2) << (int)channel << "): value " << (int)(((message->at(2) << 7) | message->at(1)) - 8192);
                break;
            default:
                break;
            }
        }

        if (nBytes == 1 && message->at(0) == 0xf8)
            std::cout << " midi clock";

        if (nBytes == 2 && (message->at(0) & 0xf0) == 0xc0) {
            uint8_t channel = message->at(0) & 0xf;
            std::cout << std::dec << "    program change (channel " << std::setw(2) << (int)channel << "): value " << std::setw(3) << (int)message->at(1);
        }

        std::cout << std::endl;
    }
}


int main(int argc, char* argv[])
{
    progName = argv[0];
    int portnum = 0;
    RtMidiIn *midiin = 0;

    // RtMidiIn constructor
    try {
        midiin = new RtMidiIn();
    }
    catch ( RtMidiError &error ) {
        error.printMessage();
        exit( EXIT_FAILURE );
    }

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if ((arg == "-h") || (arg == "--help")) {
            bye(0);
        } else if ((arg == "-l") || (arg == "--list-ports")) {
            // Check inputs.
            unsigned int nPorts = midiin->getPortCount();
            std::cout << "\nThere are " << nPorts << " MIDI input sources available.\n";
            std::string portName;
            for ( unsigned int i = 0; i < nPorts; i++ ) {
                try {
                    portName = midiin->getPortName(i);
                }
                catch ( RtMidiError &error ) {
                    error.printMessage();
                    delete midiin;
                    exit(1);
                }
                std::cout << "  Input Port " << i << ": " << portName << '\n';
            }
            delete midiin;
            exit(0);
        } else if ((arg == "-p") || (arg == "--port")) {
            if (argc > i + 1) {
                portnum = atoi(argv[i+1]);
                std::cout << "Opening Port " << portnum << std::endl;
                if (portnum >= midiin->getPortCount()) {
                    std::cerr << "Invalid Port Number!" << std::endl;
                    exit(1);
                }
            } else {
                bye(1);
            }
        }
    }

    midiin->openPort(portnum);
    // Set our callback function.  This should be done immediately after
    // opening the port to avoid having incoming messages written to the
    // queue.
    midiin->setCallback(&mycallback);
    // Don't ignore sysex, timing, or active sensing messages.
    midiin->ignoreTypes(false, false, false);

    std::cout << "Reading MIDI from port '" << midiin->getPortName(portnum) << "' press any key to quit.\n";
    char input;
    std::cin.get(input);

    delete midiin;
    return 0;
}
