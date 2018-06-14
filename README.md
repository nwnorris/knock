# knock
Arduino script for recording and recognizing knocking patterns using the LM393 mic chip.

The base algorithm polls the mic every millisecond, and compares it's analog value (1-1000) to the average value over the last 10ms. Any spikes in volume values that are a above the set threshold trigger a knock. So, as background noise increases, the Arduino doesn't constantly think knocks are being attempted. 

If another knock comes in within the defined knock timeframe, the time between the two knocks is recorded as a sequence, and so on until the knock delay has passed with no knocks occuring. Then the entire sequence is compared to the stored master sequence, within a defined margin of error (I've found ~200ms to work well). 

There's also a minimum timeout between knocks, otherwise the residual noise after the initial knock spike would still be over the knock threshold. 

Most of the constants should not need to be tweaked, but could be adjusted if you want to fine-tune the behavior for your specific situation.

I developed this code using an Arduino Mega 2560, when tested on an Uno the results were unreliable. This code is a bit meaty and isn't well suited for a small form factor. Maybe at a future date I could refactor it and improve the efficiency of it.

-Nate
