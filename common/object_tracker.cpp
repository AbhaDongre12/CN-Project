
#include "object_tracker.h"
#include <stdlib.h>

ObjectTracker::ObjectTracker()
{
  for (uint8_t i = 0; i < MAX_OBJECTS; ++i)
  {
    slots_[i] = Slot();
  }
}

void ObjectTracker::update(
    const SensorReading &r,
    unsigned long now_ms)
{
  // Invalid readings do not immediately clear existing objects.
  if (!r.valid ||
      !r.object_present ||
      r.distance_cm < 2 ||
      r.distance_cm > SensorInterface::MAX_SENSOR_DISTANCE_CM)
  {
    return;
  }

  Slot *match = findMatch(r.distance_cm, r.zone);

  if (match != nullptr)
  {
    match->distance_cm = r.distance_cm;
    match->zone = r.zone;
    match->last_seen_ms = now_ms;
    match->unconfirmed_since_ms = 0;
    return;
  }

  allocate(r.distance_cm, r.zone, now_ms);
}

uint8_t ObjectTracker::getActiveObjects(
    TrackedObject out[MAX_OBJECTS])
{
  if (out == nullptr)
  {
    return 0;
  }

  ageSlots(millis());

  uint8_t count = 0;

  for (uint8_t i = 0; i < MAX_OBJECTS; ++i)
  {
    const Slot &s = slots_[i];

    if (!s.in_use || s.pending_expire_unreported)
    {
      continue;
    }

    out[count].object_id = s.object_id;
    out[count].distance_cm = s.distance_cm;
    out[count].zone = s.zone;
    ++count;
  }

  return count;
}

uint8_t ObjectTracker::getExpiredObjects(
    uint8_t out_ids[MAX_OBJECTS])
{
  if (out_ids == nullptr)
  {
    return 0;
  }

  uint8_t count = 0;

  for (uint8_t i = 0; i < MAX_OBJECTS; ++i)
  {
    const Slot &s = slots_[i];

    if (s.in_use && s.pending_expire_unreported)
    {
      out_ids[count++] = s.object_id;
    }
  }

  return count;
}

bool ObjectTracker::confirmExpiredObjectQueued(
    uint8_t object_id)
{
  for (uint8_t i = 0; i < MAX_OBJECTS; ++i)
  {
    Slot &s = slots_[i];

    if (s.in_use &&
        s.pending_expire_unreported &&
        s.object_id == object_id)
    {
      // Release only after the sender has queued the CLEAR.
      s = Slot();
      return true;
    }
  }

  return false;
}

ObjectTracker::Slot *ObjectTracker::findMatch(
    uint16_t distance_cm,
    Zone zone)
{
  (void)distance_cm;

  // Each sensor must have a unique zone.
  // Retain its object ID even when the distance changes sharply.
  for (uint8_t i = 0; i < MAX_OBJECTS; ++i)
  {
    Slot &s = slots_[i];

    if (!s.in_use || s.pending_expire_unreported)
    {
      continue;
    }

    if (s.zone == zone)
    {
      return &s;
    }
  }

  return nullptr;
}

void ObjectTracker::allocate(
    uint16_t distance_cm,
    Zone zone,
    unsigned long now_ms)
{
  for (uint8_t i = 0; i < MAX_OBJECTS; ++i)
  {
    Slot &s = slots_[i];

    // Never overwrite a tracked or pending-expiry object.
    if (s.in_use)
    {
      continue;
    }

    s = Slot();
    s.in_use = true;
    s.object_id = next_id_++;

    if (next_id_ == 0)
    {
      next_id_ = 1;
    }

    // Avoid an ID collision after the counter wraps.
    bool collision = true;

    while (collision)
    {
      collision = false;

      for (uint8_t j = 0; j < MAX_OBJECTS; ++j)
      {
        if (j != i &&
            slots_[j].in_use &&
            slots_[j].object_id == next_id_)
        {
          collision = true;
          break;
        }
      }

      if (collision)
      {
        ++next_id_;

        if (next_id_ == 0)
        {
          next_id_ = 1;
        }
      }
    }

    s.distance_cm = distance_cm;
    s.zone = zone;
    s.last_seen_ms = now_ms;
    s.unconfirmed_since_ms = 0;
    s.pending_expire_unreported = false;

    return;
  }

  // If all slots are occupied, do not overwrite an existing object.
}

void ObjectTracker::ageSlots(unsigned long now_ms)
{
  for (uint8_t i = 0; i < MAX_OBJECTS; ++i)
  {
    Slot &s = slots_[i];

    if (!s.in_use || s.pending_expire_unreported)
    {
      continue;
    }

    const unsigned long age = now_ms - s.last_seen_ms;

    if (age > GRACE_START_MS &&
        s.unconfirmed_since_ms == 0)
    {
      s.unconfirmed_since_ms = now_ms;
    }

    if (s.unconfirmed_since_ms != 0 &&
        now_ms - s.unconfirmed_since_ms > GRACE_DURATION_MS)
    {
      s.pending_expire_unreported = true;
    }
  }
}
