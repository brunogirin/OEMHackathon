# Data platform hack ideas

## Context

Both OEM and OpenTRV are deployed in a similar way: a number of sensor nodes
send data to a central concentrator installed in the building where you have
access to all the data collected around the building:

    +--------+     +--------------+
    | Device |---->| Concentrator |
    +--------+     +--------------+

This is great for single building deployments. However, in environments
where you want to deploy equipment in multiple buildings, such as social
housing associations or commercial building managers, you may want to forward
the data from multiple buildings to a data platform in the cloud or in a
data centre, where you can compare data from several buildings and manage the
equipment centrally:

    +--------+     +--------------+      (-----)      +---------------+
    | Device |---->| Concentrator |---->( Cloud )---->| Data Platform |
    +--------+     +--------------+      (-----)      +---------------+

The protocol and data format between the Concentrator and the Data Platform
is different from the one between the Device and the Concentrator as it needs
to travel over the Internet and is less constrained in terms of bandwidth.

## Problems to solve

### Payload format

Open source and commercial products today tend to have their own data format
to send data from devices to cloud platforms. Although there are emerging
standards like [SenML](https://github.com/fluffy/senml-spec), they are not
widely implemented, do not include any meta-data and delegate security to the
underlying protocol, such as HTTPS or MQTT. In addition, SenML does not
really support cumulative data series, such as kWh energy values or pulse data
that can be sent by energy monitoring equipment like OEM. Possible problems to
tackle:

1. __Basic payload__: propose a payload data format and build a simple code
   example.
2. __Energy__: add support for cumulative data series such as energy.
3. __Non repudiation__: it would be great if we could ensure that data comes
   from the expected sensor and hasn't been tampered with. Message signing
   may provide a solution: see
   [JSON Web Signature](https://tools.ietf.org/html/rfc7515)

See [HyperCat](http://www.hypercat.io/) for ways to integrate SenML in wider
contexts.

See [CoAP](https://tools.ietf.org/html/rfc7252) for a lightweight protocol
standard that natively uses UDP but can be proxied over HTTP.

### Commissioning

When you deal with a large number of sensors and each of them has a complex
installation procedure, it can become very time consuming. There are standard
protocols to deal with commissioning such as
[TR-069](https://en.wikipedia.org/wiki/TR-069) but they are very cumbersome
and are designed for device management on a LAN, not over the internet.
Possible problems to tackle:

1. __Commissioning__: define an interaction process to simplify commissioning
   of equipment so that the configuration on the concentrator is very simple
   and most configuration is done through exchanges between concentrator and
   data platform.
2. __Secure commissioning__: define how the commissioning interaction process
   should be updated to handle security credentials such as shared keys. One
   example of such an interaction is the way Vagrant starts a new VM from a
   clean image by replacing a temporary unsafe SSH key with a safe one as soon
   as the VM is brought up the first time round.
3. __Configuration changes__: define an interaction process to handle
   configuration changes on the concentrator or data platform and have it
   pushed to the other party, e.g. the addition of a new sensor.
4. __Sensor calibration__: define an interaction process to handle sensor
   calibration on commissioning.

In order not to re-invent the wheel, it might be good to take inspiration from
TR-069 in terms of interaction while coming up with a more lightweight process
and using JSON instead of XML.

### Device management

Once a large number of devices are installed, some of them will stop working
or will need some sort of maintenance. It would be useful to make the data
platform aware of devices that need attention. Possible problems to tackle:

1. __Device status information__: work out what useful status information
   could be sent to the data platform in order to have an overall view of
   whether devices and sensors are operating correctly.
2. __Alarms__: work out how alarms could be propagated to the data platform
   so that they can be treated centrally.

## Notes

### Why is energy and resource consumption data complicated?

Energy and resource consumption differs from classic sensors in the fact that
they are cumulative data series. If you draw 1 litre of water in 1 hour, then
over 2 hours you draw 2 litres of water; this has a number of implications:

* When rolling up data (e.g. going from an hourly resolution to daily), you
  need to sum all data points rather than take the average;
* A consumption value is only relevant if you also know the period over which
  it was measured.

One typical way to resolve that problem is to use a cumulative total count as
a data series, that is a value that always increases, which is exactly how
traditional energy meters or the mileage meter on a car work. To obtain the
consumption for a given period you then need to calculate the difference
between the value of the total count at the end of the period and its value at
the beginning of the period. The consequence of this is that you always need
an initial reference data point.

Another way to count energy or resource usage is to count pulses, which is
typically what happens with gas meters: each time a m3 of gas is used for
instance, a new pulse is generated. This pulse is like saying "add 1" to the
total count. This method has the same limits as the total count in terms of
having an initial reference point. In addition, it has the downside that if
you fail to receive any of the pulses, your resulting total count is incorrect.
