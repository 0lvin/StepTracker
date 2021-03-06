Utility for dump ibody tracker state.

All this packages received by monitoring usb traffic between tracker and
windows application. Windows application run inside wm on linux machine.
And all packages have dumped by read /sys/kernel/debug/usb/usbmon/{bus_id}u.
For get such bus id can be used output of lsusb.

Note: All information received without any kind of reverse engineering for
software/hardware products.

# Operations:

## Status:

```
-> 5a 42|00 00 00 00 00 00 00 00 00 00 00 00 00 00|9c
    ^     ^                                        ^
    |     |                                        hash sum
    |     empty in general
    operation code
<- 5a 42|00 22 aa 54 45|xx xx xx xx xx xx|00 00 00|f0
    ^     ^              ^                          ^
    |     |              |                          hash sum
    |     |              tracker id
    |     unknow
    operation code
```

## Log size:

```
-> 5a 46|00 00 00 00 00 00 00 00 00 00 00 00 00 00|a0
    ^     ^                                        ^
    |     |                                        hash sum
    |     empty in general
    operation code

<- 5a 46|00 00 00 03|00 00 00 00 00 00 00 00 00 00|a3
<- 5a 46|7f ff ff ff|00 00 00 00 00 00 00 00 00 00|a3
    ^     ^           ^                            ^
    |     |           |                            hash sum
    |     |           unknow
    |     days bit mask, if more that 28 days, need to do cleanup,
    |     need retest - sure only for 1 byte
    operation code
```

## Day dump(run's after 0x46):

```
-> 5a 43|00|00 00 00 00 00 00 00 00 00 00 00 00 00|9d
    ^     ^  ^                                     ^
    |     |  |                                     hash sum
    |     |   empty in general
    |     days back, 0 - today, 1 yesterday
    operation code
<- 5a 43|f0|15|07|18|39|00|00 00|00 00|00 00|00 00|fa
    ^     ^  ^  ^  ^  ^  ^  ^     ^     ^    ^     ^
    |     |  |  |  |  |  |  |     |     |    |    hash sum
    |     |  |  |  |  |  |  |     |     |    some useful data?
    |     |  |  |  |  |  |  |     |     m, distance if "wake"(little endian)
    |     |  |  |  |  |  |  |     count steps if "wake"(little endian)
    |     |  |  |  |  |  |  kkal, energy if "wake" (little endian)
    |     |  |  |  |  |  0 - wake, 0xff - sleep
    |     |  |  |  |  15 min interval number from midnight (96 records for day)
    |     |  |  |  day(in bcd format)
    |     |  |  month(in bcd format)
    |     |  year(in bcd format)
    |     exist some data
    operation code
<- 5a 43|f0|15|07|22|5d|ff|00 00 00 00 0e 29 11 0d|7c
    ^     ^  ^  ^  ^  ^  ^  ^                      ^
    |     |  |  |  |  |  |  |                     hash sum
    |     |  |  |  |  |  |  one byte for every 112.5 seconds (15 min / 8), if "sleep"
    |     |  |  |  |  |  0 - wake, 0xff - sleep
    |     |  |  |  |  15 min interval number from midnight (96 records for day)
    |     |  |  |  day(in bcd format)
    |     |  |  month(in bcd format)
    |     |  year(in bcd format)
    |     exist some data
    operation code
<- 5a 43|00 ff|00 00 00 00 00 00 00 00 00 00 00 00|9c
    ^     ^  ^  ^                                  ^
    |     |  |  |                                  hash sum
    |     |  |  in general empty
    |     for 00ff - no data for day
    operation code
```

## Clean up day in logs:

```
-> 5a 04|01|00 00 00 00 00 00 00 00 00 00 00 00 00|5f
    ^     ^  ^                                     ^
    |     |  |                                     hash sum
    |     |   empty in general
    |     days back, 0 - today, 1 yesterday
    operation code
<- 5a 04|00 00 00 00 00 00 00 00 00 00 00 00 00 00|5e
    ^     ^                                        ^
    |     |                                        hash sum
    |     in general empty
    operation code
```

## Set time:

```
-> 5a 01|15|07|11|17|09|05|00 00 00 00 00 00 00 00|ad
    ^     ^  ^  ^  ^  ^  ^  ^                      ^
    |     |  |  |  |  |  |  |                      hash sum
    |     |  |  |  |  |  |  empty in general
    |     |  |  |  |  |  seconds(in bcd format)
    |     |  |  |  |  minutes(in bcd format)
    |     |  |  |  hour(in bcd format)
    |     |  |  day(in bcd format)
    |     |  month(in bcd format)
    |     year(in bcd format)
    operation code

<- 5a 2e|00 00 00 00 00 00 00 00 00 00 00 00 00 00|88
    ^     ^                                        ^
    |     |                                        hash sum
    |     empty in general
    operation code
```

## Reset (never returns result):

```
-> 5a 2e|00 00 00 00 00 00 00 00 00 00 00 00 00 00|88
    ^     ^                                        ^
    |     |                                        hash sum
    |     empty in general
    operation code
```

# Request examples:
 * time - 1 record responce
 * 5a011507 11170905 00000000 00000000 ad
 * 5a011507 11170909 00000000 00000000 b1
 * 5a011507 11171217 00000000 00000000 c8
 * 5a011507 11171226 00000000 00000000 d7
 * 5a011507 16081455 00000000 00000000 fe
 * 5a011507 16081510 00000000 00000000 ba
 * 5a011507 16081546 00000000 00000000 f0
 * reset - no responce
 * 5a2e0000 00000000 00000000 00000000 88
 * status  - 1 record responce
 * 5a420000 00000000 00000000 00000000 9c
 * log size - 1 record responce
 * 5a460000 00000000 00000000 00000000 a0
 * cleanup one day - 1 record responce
 * 5a040100 00000000 00000000 00000000 5f
 * some log with 15 minutes intervals 1 or 96 responces:
 * 5a430000 00000000 00000000 00000000 9d

# port setings
 * 9600 bps, 8n1

# hash function

```
unsigned char hash_sum(const char* buffer, size_t size) {
    unsigned int crc = 0;
    int i = 0;
    for (i = 0; i < size; i++) {
        crc = (crc + buffer[i]) % 0x100;
    }
    return crc & 0xff;
}
```

# ToDo:
 * undestand bytes before tracker id (5 bytes)
