# OpenHaystack

OpenHaystack is a framework for tracking personal Bluetooth devices via Apple's massive Find My network. Use it to create your own tracking _tags_ that you can append to physical objects (keyrings, backpacks, ...) or integrate it into other Bluetooth-capable devices such as notebooks.


OpenHaystack requires macOS 11 (Big Sur).

### Installation

The OpenHaystack application requires a custom plugin for Apple Mail. It is used to download location reports from Apple's servers via a private API (technical explanation: the plugin inherits Apple Mail's entitlements required to use this API).
Therefore, the installation procedure is slightly different and requires you to temporarily disable [Gatekeeper](https://support.apple.com/guide/security/gatekeeper-and-runtime-protection-sec5599b66df/1/web/1).
Our plugin does not access any other private data such as emails (see [source code](OpenHaystack/OpenHaystackMail)).

1. Download a precompiled binary release from our <a href="https://github.com/seemoo-lab/openhaystack/releases">GitHub page</a>.  
   _Alternative:_ build the application from source via Xcode.
2. Open OpenHaystack. This will ask you to install the Mail plugin in `~/Library/Mail/Bundle`.
3. Open a terminal and run `sudo spctl --master-disable`, which will disable Gatekeeper and allow our Apple Mail plugin to run.
4. Open Apple Mail. Go to _Preferences_ → _General_ → _Manage Plug-Ins..._ and activate the checkbox next to _OpenHaystackMail.mailbundle_.
   * If the _Manage Plug-Ins..._ button does not appear. Run this command in terminal `sudo defaults write "/Library/Preferences/com.apple.mail" EnableBundles 1`
5. Allow access and restart Mail.
6. Open a terminal and enter `sudo spctl --master-enable`, which will enable Gatekeeper again.



## How to track other Bluetooth devices?

In principle, any Bluetooth device can be turned into an OpenHaystack accessory that is trackable via Apple's Find My network.
Currently, we provide a convenient deployment method of our OpenHaystack firmwares for a small number of embedded devices (see table below). We also support Linux devices via our generic HCI script.
Feel free to port OpenHaystack to other devices that support Bluetooth Low Energy based on the [source code of our firmware](Firmware) and the specification in [our paper](#references). Please share your results with us!

| Platform | Tested on | Deploy via app | Comment |
|----------|-----------|:--------------:|---------|
| [Nordic nRF51](Firmware/Microbit_v1) | BBC micro:bit v1 | ✓ | Only supports nRF51822 at this time (see issue #6). |
| [Espressif ESP32](Firmware/ESP32) | SP32-WROOM, ESP32-WROVER | ✓ | Deployment can take up to 3 minutes. Requires Python 3. Thanks **@fhessel**. |
| [Linux HCI](Firmware/Linux_HCI) | Raspberry Pi 4 w/ Raspbian | | Should support any Linux machine. |

![Setup](Resources/Setup.jpg)




## References

- Alexander Heinrich, Milan Stute, Tim Kornhuber, Matthias Hollick. **Who Can _Find My_ Devices? Security and Privacy of Apple's Crowd-Sourced Bluetooth Location Tracking System.** _Proceedings on Privacy Enhancing Technologies (PoPETs)_, 2021. [doi:10.2478/popets-2021-0045](https://doi.org/10.2478/popets-2021-0045) [📄 Paper](https://www.petsymposium.org/2021/files/papers/issue3/popets-2021-0045.pdf) [📄 Preprint](https://arxiv.org/abs/2103.02282).
- Alexander Heinrich, Milan Stute, and Matthias Hollick. **DEMO: OpenHaystack: A Framework for Tracking Personal Bluetooth Devices via Apple’s Massive Find My Network.** _14th ACM Conference on Security and Privacy in Wireless and Mobile (WiSec ’21)_, 2021.
- Tim Kornhuber. **Analysis of Apple's Crowd-Sourced Location Tracking System.** _Technical University of Darmstadt_, Master's thesis, 2020.
- Apple Inc. **Find My Network Accessory Specification – Developer Preview – Release R3.** 2020. [📄 Download](https://developer.apple.com/find-my/).


