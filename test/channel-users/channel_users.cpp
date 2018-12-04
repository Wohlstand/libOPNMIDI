#include <catch.hpp>
#include <string>
#include <vector>

#include "opnmidi_midiplay.hpp"
#include "opnmidi_opn2.hpp"
#include "opnmidi_private.hpp"

typedef OPNMIDIplay::OpnChannel Channel;
typedef Channel::Location Location;
typedef Channel::LocationData LocationData;
typedef Channel::users_iterator users_iterator;
typedef Channel::const_users_iterator const_users_iterator;

static size_t iterated_size(const Channel &channel)
{
    size_t size = 0;
    for (const_users_iterator i = channel.users.begin(); !i.is_end(); ++i)
        ++size;
    return size;
}

bool consistent_size(const Channel &channel)
{
    return channel.users.size() == iterated_size(channel);
}

TEST_CASE("[OPNMIDIplay::OpnChannel] User list: initialization")
{
    Channel channel;
    REQUIRE(channel.users.size() == 0);
    REQUIRE(consistent_size(channel));
}

TEST_CASE("[OPNMIDIplay::OpnChannel] User list: appending")
{
    Channel channel;

    // insert
    users_iterator user1 = channel.find_or_create_user(Location{0, 1, {0}});
    REQUIRE(!user1.is_end());
    REQUIRE(&channel.users.back() == &*user1);
    REQUIRE(&channel.users.front() == &*user1);
    REQUIRE(channel.users.size() == 1);
    REQUIRE(consistent_size(channel));
    REQUIRE(!channel.find_user(Location{0, 1, {0}}).is_end());

    // try insert with same note number as previous
    user1 = channel.find_or_create_user(Location{0, 1, {0}});
    REQUIRE(!user1.is_end());
    REQUIRE(channel.users.size() == 1);
    REQUIRE(consistent_size(channel));
    REQUIRE(!channel.find_user(Location{0, 1, {0}}).is_end());

    // insert
    users_iterator user2 = channel.find_or_create_user(Location{1, 0, {0}});
    REQUIRE(!user2.is_end());
    REQUIRE(channel.users.size() == 2);
    REQUIRE(consistent_size(channel));
    REQUIRE(!channel.find_user(Location{1, 0, {0}}).is_end());

    // try insert with same channel number as previous
    user2 = channel.find_or_create_user(Location{1, 0, {0}});
    REQUIRE(!user2.is_end());
    REQUIRE(channel.users.size() == 2);
    REQUIRE(consistent_size(channel));
    REQUIRE(!channel.find_user(Location{1, 0, {0}}).is_end());
}

TEST_CASE("[OPNMIDIplay::OpnChannel] User list: erasing at the back")
{
    Channel channel;

    users_iterator user3 = channel.find_or_create_user(Location{3, 3, {0}});
    users_iterator user2 = channel.find_or_create_user(Location{2, 2, {0}});
    users_iterator user1 = channel.find_or_create_user(Location{1, 1, {0}});
    REQUIRE(!user1.is_end());
    REQUIRE(!user2.is_end());
    REQUIRE(!user3.is_end());
    REQUIRE(&channel.users.back() == &*user1);
    REQUIRE(&channel.users.front() == &*user3);
    REQUIRE(channel.users.size() == 3);
    REQUIRE(consistent_size(channel));

    // erase at the back
    channel.users.erase(user1);
    REQUIRE(&channel.users.back() == &*user2);
    REQUIRE(&channel.users.front() == &*user3);
    REQUIRE(channel.users.size() == 2);
    REQUIRE(consistent_size(channel));
}

TEST_CASE("[OPNMIDIplay::OpnChannel] User list: erasing at the front")
{
    Channel channel;

    users_iterator user3 = channel.find_or_create_user(Location{3, 3, {0}});
    users_iterator user2 = channel.find_or_create_user(Location{2, 2, {0}});
    users_iterator user1 = channel.find_or_create_user(Location{1, 1, {0}});
    REQUIRE(!user1.is_end());
    REQUIRE(!user2.is_end());
    REQUIRE(!user3.is_end());
    REQUIRE(&channel.users.back() == &*user1);
    REQUIRE(&channel.users.front() == &*user3);
    REQUIRE(channel.users.size() == 3);
    REQUIRE(consistent_size(channel));

    // erase at the front
    channel.users.erase(user3);
    REQUIRE(&channel.users.back() == &*user1);
    REQUIRE(&channel.users.front() == &*user2);
    REQUIRE(channel.users.size() == 2);
    REQUIRE(consistent_size(channel));
}

TEST_CASE("[OPNMIDIplay::OpnChannel] User list: erasing in the middle")
{
    Channel channel;

    users_iterator user3 = channel.find_or_create_user(Location{3, 3, {0}});
    users_iterator user2 = channel.find_or_create_user(Location{2, 2, {0}});
    users_iterator user1 = channel.find_or_create_user(Location{1, 1, {0}});
    REQUIRE(!user1.is_end());
    REQUIRE(!user2.is_end());
    REQUIRE(!user3.is_end());
    REQUIRE(&channel.users.back() == &*user1);
    REQUIRE(&channel.users.front() == &*user3);
    REQUIRE(channel.users.size() == 3);
    REQUIRE(consistent_size(channel));

    // erase at the front
    channel.users.erase(user2);
    REQUIRE(&channel.users.back() == &*user1);
    REQUIRE(&channel.users.front() == &*user3);
    REQUIRE(channel.users.size() == 2);
    REQUIRE(consistent_size(channel));
}

TEST_CASE("[OPNMIDIplay::OpnChannel] User list: copy constructor")
{
    Channel channel1;

    users_iterator user3 = channel1.find_or_create_user(Location{3, 3, {0}});
    users_iterator user2 = channel1.find_or_create_user(Location{2, 2, {0}});
    users_iterator user1 = channel1.find_or_create_user(Location{1, 1, {0}});
    REQUIRE(!user1.is_end());
    REQUIRE(!user2.is_end());
    REQUIRE(!user3.is_end());
    REQUIRE(&channel1.users.back() == &*user1);
    REQUIRE(&channel1.users.front() == &*user3);
    REQUIRE(channel1.users.size() == 3);
    REQUIRE(consistent_size(channel1));

    //
    Channel channel2(channel1);
    REQUIRE(channel2.users.size() == 3);
    REQUIRE(consistent_size(channel2));

    users_iterator i1 = channel1.users.begin();
    users_iterator i2 = channel2.users.begin();
    for(size_t i = 0; i < 3; ++i) {
        REQUIRE(i1 != i2);
        REQUIRE(i1->value.loc == i2->value.loc);
        i1 = i1->next;
        i2 = i2->next;
    }
}

TEST_CASE("[OPNMIDIplay::OpnChannel] User list: copy assignment")
{
    Channel channel1;

    users_iterator user3 = channel1.find_or_create_user(Location{3, 3, {0}});
    users_iterator user2 = channel1.find_or_create_user(Location{2, 2, {0}});
    users_iterator user1 = channel1.find_or_create_user(Location{1, 1, {0}});
    REQUIRE(!user1.is_end());
    REQUIRE(!user2.is_end());
    REQUIRE(!user3.is_end());
    REQUIRE(&channel1.users.back() == &*user1);
    REQUIRE(&channel1.users.front() == &*user3);
    REQUIRE(channel1.users.size() == 3);
    REQUIRE(consistent_size(channel1));

    //
    Channel channel2 = channel1;
    REQUIRE(channel2.users.size() == 3);
    REQUIRE(consistent_size(channel2));

    users_iterator i1 = channel1.users.begin();
    users_iterator i2 = channel2.users.begin();
    for(size_t i = 0; i < 3; ++i) {
        REQUIRE(i1 != i2);
        REQUIRE(i1->value.loc == i2->value.loc);
        i1 = i1->next;
        i2 = i2->next;
    }
}

TEST_CASE("[OPNMIDIplay::OpnChannel] User list: appending full")
{
    Channel channel;
    Location loc = Location{0, 0, {0}};

    // insert users up to capacity
    for(unsigned i = 0; i < channel.users.capacity(); ++i) {
        users_iterator user = channel.find_or_create_user(loc);
        REQUIRE(!user.is_end());
        // increment location
        ++loc.note;
        if(loc.note == 128) {
            ++loc.MidCh;
            loc.note = 0;
        }
    }
    REQUIRE(channel.users.size() == channel.users.capacity());
    REQUIRE(consistent_size(channel));

    // attempt new insertion
    users_iterator user = channel.find_or_create_user(loc);
    REQUIRE(user.is_end());

    // free a slot and retry
    channel.users.erase(channel.users.begin());
    user = channel.find_or_create_user(loc);
    REQUIRE(!user.is_end());
    REQUIRE(channel.users.size() == channel.users.capacity());
    REQUIRE(consistent_size(channel));
}

TEST_CASE("[OPNMIDIplay::OpnChannel] User list: assigning empty lists")
{
    Channel channel1;
    REQUIRE(channel1.users.size() == 0);
    REQUIRE(consistent_size(channel1));

    Channel channel2;
    REQUIRE(channel2.users.size() == 0);
    REQUIRE(consistent_size(channel2));

    channel1 = channel2;
    REQUIRE(channel1.users.size() == 0);
    REQUIRE(consistent_size(channel1));
}
