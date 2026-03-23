/*

1 writer - reader is consistent
- producer placing data into one end of an array, while the consumer removes data from the other
- When the end of the array is
reached, the producer wraps backaround to the beginning. So a circular buffer
requires an array and two index values to trackwhere the next new value goes and
which value should be removed from the buffer next.
*/

#include <linux/kfifo.h>