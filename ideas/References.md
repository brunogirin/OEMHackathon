# References:

### Why is energy and resource consumption data complicated?

* RRDTool - well known project which has tackled cumulative data series
  * http://oss.oetiker.ch/rrdtool/
  * https://github.com/oetiker/rrdtool-1.x
  
    Has the idea of: Data source types: GAUGE, COUNTER and others
    
    GAUGE: is for things like temperatures or number of people in a room or the value of a RedHat share
    GAUGE: does not save the rate of change. It saves the actual value itself.

    COUNTER: is for continuous incrementing counters like the ifInOctets counter in a router
    COUNTER: will save the rate of change of the value over a step period.

    The COUNTER data source assumes that the counter never decreases, except when a counter overflows.
    The update function takes the overflow into account.
    The counter is stored as a per-second rate.
    When the counter overflows, RRDtool checks if the overflow happened at the 32bit or 64bit border and acts accordingly
    by adding an appropriate value to the result.
    
    Previously used in similar projects:
    * RRDtool and energy
        * http://www.jibble.org/currentcost/ 
        * http://knolleary.net/2010/12/26/power-graphs-again/ 

    * RRDtool and MQTT
        * https://www.clusterfsck.io/blog/mqtt-to-rrd-daemon/
        * https://www.clusterfsck.io/blog/visualizing-rrd-data-using-nvd3/
        * http://www.futuretab.com/?p=207

    * User experience - RRDtool (subsequently replaced by Node-Red) MQTT, EmonCMS
        * https://github.com/flabbergast/rrd-pi-sensors
 
 ### Device management
 
 * RRDTool and Cacti http://www.cacti.net/index.php
    Potential (temporary) use for testing
