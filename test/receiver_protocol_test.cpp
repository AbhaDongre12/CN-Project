
#include <cassert>
#include <cstdint>
#include <iostream>
#include "../receiver/ReceiverProtocol.h"

using namespace std;

void test_full_packet()
{
    uint8_t packet[9] = {0x01, 1, 1, 0x03, 30, 0, 2, 0, 1};
    BlindLinkMessage msg{};

    assert(ReceiverProtocol::decode(packet, 9, &msg));
    assert(msg.type == MESSAGE_FULL);
    assert(msg.vehicle_id == 1);
    assert(msg.object_id == 1);
    assert(msg.distance_cm == 30);
    assert(msg.zone == ZONE_REAR);

    cout << "PASS: FULL packet\n";
}

void test_delta_packet()
{
    uint8_t packet[9] = {0x02, 1, 1, 0x01, 45, 0, 0, 0, 2};
    BlindLinkMessage msg{};

    assert(ReceiverProtocol::decode(packet, 9, &msg));
    assert(msg.type == MESSAGE_DELTA);
    assert(msg.distance_cm == 45);

    cout << "PASS: DELTA packet\n";
}

void test_clear_packet()
{
    uint8_t packet[9] = {0x03, 1, 1, 0, 0, 0, 0, 0, 3};
    BlindLinkMessage msg{};

    assert(ReceiverProtocol::decode(packet, 9, &msg));
    assert(msg.type == MESSAGE_CLEAR);

    cout << "PASS: CLEAR packet\n";
}

void test_heartbeat_packet()
{
    uint8_t packet[9] = {0x04, 1, 0, 0, 0, 0, 0, 0, 4};
    BlindLinkMessage msg{};

    assert(ReceiverProtocol::decode(packet, 9, &msg));
    assert(msg.type == MESSAGE_HEARTBEAT);

    cout << "PASS: HEARTBEAT packet\n";
}

void test_invalid_packets()
{
    BlindLinkMessage msg{};

    uint8_t bad_length[8] = {0x01, 1, 1, 3, 30, 0, 2, 0};
    assert(!ReceiverProtocol::decode(bad_length, 8, &msg));

    uint8_t bad_distance[9] = {0x01, 1, 1, 3, 9, 0, 2, 0, 1};
    assert(!ReceiverProtocol::decode(bad_distance, 9, &msg));

    uint8_t bad_zone[9] = {0x01, 1, 1, 3, 30, 0, 4, 0, 1};
    assert(!ReceiverProtocol::decode(bad_zone, 9, &msg));

    uint8_t bad_type[9] = {0x06, 1, 1, 3, 30, 0, 2, 0, 1};
    assert(!ReceiverProtocol::decode(bad_type, 9, &msg));

    uint8_t ack[9] = {0x05, 1, 1, 0, 0, 0, 0, 0, 1};
    assert(!ReceiverProtocol::decode(ack, 9, &msg));

    cout << "PASS: Invalid packets rejected\n";
}

int main()
{
    test_full_packet();
    test_delta_packet();
    test_clear_packet();
    test_heartbeat_packet();
    test_invalid_packets();

    cout << "\nAll protocol tests passed!\n";
    return 0;
}
