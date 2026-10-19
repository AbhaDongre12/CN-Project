
#include <unity.h>
#include <cstdint>
#include "../receiver/ReceiverProtocol.h"

void test_full_packet()
{
    uint8_t packet[9] = {0x01, 1, 1, 0x03, 30, 0, 2, 0, 1};
    BlindLinkMessage msg{};

    TEST_ASSERT_TRUE(ReceiverProtocol::decode(packet, 9, &msg));
    TEST_ASSERT_EQUAL(MESSAGE_FULL, msg.type);
    TEST_ASSERT_EQUAL(1, msg.vehicle_id);
    TEST_ASSERT_EQUAL(1, msg.object_id);
    TEST_ASSERT_EQUAL(30, msg.distance_cm);
    TEST_ASSERT_EQUAL(ZONE_REAR, msg.zone);
}

void test_delta_packet()
{
    uint8_t packet[9] = {0x02, 1, 1, 0x01, 45, 0, 0, 0, 2};
    BlindLinkMessage msg{};

    TEST_ASSERT_TRUE(ReceiverProtocol::decode(packet, 9, &msg));
    TEST_ASSERT_EQUAL(MESSAGE_DELTA, msg.type);
    TEST_ASSERT_EQUAL(45, msg.distance_cm);
}

void test_clear_packet()
{
    uint8_t packet[9] = {0x03, 1, 1, 0, 0, 0, 0, 0, 3};
    BlindLinkMessage msg{};

    TEST_ASSERT_TRUE(ReceiverProtocol::decode(packet, 9, &msg));
    TEST_ASSERT_EQUAL(MESSAGE_CLEAR, msg.type);
}

void test_heartbeat_packet()
{
    uint8_t packet[9] = {0x04, 1, 0, 0, 0, 0, 0, 0, 4};
    BlindLinkMessage msg{};

    TEST_ASSERT_TRUE(ReceiverProtocol::decode(packet, 9, &msg));
    TEST_ASSERT_EQUAL(MESSAGE_HEARTBEAT, msg.type);
}

void test_invalid_packets()
{
    BlindLinkMessage msg{};

    uint8_t bad_length[8] = {0x01, 1, 1, 3, 30, 0, 2, 0};
    TEST_ASSERT_FALSE(ReceiverProtocol::decode(bad_length, 8, &msg));

    uint8_t bad_distance[9] = {0x01, 1, 1, 3, 9, 0, 2, 0, 1};
    TEST_ASSERT_FALSE(ReceiverProtocol::decode(bad_distance, 9, &msg));

    uint8_t bad_zone[9] = {0x01, 1, 1, 3, 30, 0, 4, 0, 1};
    TEST_ASSERT_FALSE(ReceiverProtocol::decode(bad_zone, 9, &msg));

    uint8_t bad_type[9] = {0x06, 1, 1, 3, 30, 0, 2, 0, 1};
    TEST_ASSERT_FALSE(ReceiverProtocol::decode(bad_type, 9, &msg));

    uint8_t ack[9] = {0x05, 1, 1, 0, 0, 0, 0, 0, 1};
    TEST_ASSERT_FALSE(ReceiverProtocol::decode(ack, 9, &msg));
}

void setUp(void) {}
void tearDown(void) {}

int main(int argc, char **argv)
{
    UNITY_BEGIN();

    RUN_TEST(test_full_packet);
    RUN_TEST(test_delta_packet);
    RUN_TEST(test_clear_packet);
    RUN_TEST(test_heartbeat_packet);
    RUN_TEST(test_invalid_packets);

    return UNITY_END();
}
