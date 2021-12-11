# STM32F4 Discovery ADC to I2S Using DMA

The STM32F4 Discovery boasts a Cirrus Logic CS43L22 DAC and an audio output jack, so it's only natural you'd want to use that to output some sound.
The ADCs on the STM32F407 aren't really studio quality, but it's worth a try. Maybe you want that vintage 12 bit sound, or sampling a speech signal and you don't need those "extra" bits...

This project samples an ADC channel @44kHz and outputs the audio to the onboard audio jack.
It uses DMA, so you can process the samples in blocks and do other stuff as well.

The project is in STM32CubeIDE and uses ST's HAL drivers. CubeMX is used to configure the peripherals.
It also uses a modified version of the CS43L22 driver provided in the BSP for the discovery board.

Please note that the output is a mono sound, so if your favourite song seems weird when you play it through this, remember that you might be hearing only the left channel.

## IP Configuration
The ADC is configured to be triggered by a timer. The timer is where we ensure 44000 update events per second. ADC is setup to have 12 bits resolution.
I2C and I2S are necessary for controlling and streaming audio to the CS43L22.
In particular the I2S provides a master clock signal to the CS43L22.
DMA streams for ADC and I2S are enabled, and configured for burst mode.
I use the DAC to output a fixed voltage to add to the audio signal. I don't substract this voltage later on because the audio DAC seems to do it.

## The application
The STM32 DMA provides a "transfer half-complete" and a "transfer complete" interrupts. 
My first intuition about this was to set the ADC's DMA to circular mode, and have the following steps: 
* The ADC acquires a first block of samples using DMA
* Transfer half-complete interrupt of the ADC's DMA channel is triggered. We use it to start the I2S DMA transfer of the first block.
* The ADC acquires a second block of samples
* Transfer complete interrupt is triggered. We use it to start the I2S DMA transfer of the second block.

Unfortunately, there was a cracking sound. I suspect it was due the I2S master clock (MCK) stopping between blocks.
This was not a problem when I used polling mode, acquiring and sending one simple at a time. Using a logic analyzer, these stops happened at about 17kHz in polling mode. In DMA mode, since we're using blocks, they happen much less frequently and are audible.

My second attempt was to set both the ADC and I2S DMA channels to circular mode and do the following
* Start ADC in DMA mode to acquire 2*N samples
* When In the first time ADC DMA's transfer half-complete interrupt happens launch the I2S transfer of 2*N samples
* **Wish** that the transfers remain magically synchronized, so that when the ADC is acquiring the first block (N samples), I2S would be transmitting the second block, and so on...

STM32 said "Nah, aint gonna happen."
What eventually happened is that the output was clear for a while, and due to the unavoidable difference between the time it takes for the ADC to acquire a block and the time the I2S takes to send it, the sound became noisy, and then clear again, and then noisy. Basically the I2S transfer caught on to the ADC conversion, and both the ADC and I2S operate on the same block.

Since I needed the I2S MCLK clock to be ON all the time. I set the I2S DMA channel to circular, the ADC DMA channel to normal and had the STM operate as follows
* 1- I2S starts sending (buffers are initialized to 0)
* 2- When the half transfer interrupt of the I2S is triggered, launch the ADC to acquire the first block of samples.
* 3- I2S starts sending the second block, at this time still 0s. 
* 4- The I2S DMA channels transfer complete interrupt occurs. Start the ADC aqcuisition of the second block of samples.
* 5- Back to step 1

The second time around the buffers won't be 0s, and there will be sound. 
If you managed to synchronize the DMA transfers some other way and had a clear sound, please let me know.

## The result
The sound quality isn't that good, and there is audible noise, but hey at least DMA worked!



