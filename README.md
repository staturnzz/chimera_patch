# chimera_patch

This patch was created to improve the success rate on 4k devices. The exploit used is a fixed version of [time_waste](https://github.com/jakeajames/time_waste) by Jake James, I'm calling *time_saved*. While is this mainly for 4k devices it *should* work on any device supported by Chimera. Use at your own risk, no support will be given.

### Usage:
```bash
git clone https://github.com/staturnzz/chimera_patch.git
cd chimera_patch
make all
./patch_ipa <stock.ipa> <patched.ipa>
```

Then sideload your `patched.ipa` and enjoy. 
**I will never release an IPA so don't ask.**

### Credits:
- [Jake James](https://github.com/jakeajames) for time_waste
- [Coolstar](https://github.com/coolstar) for Chimera
- [Odyssey Team, et al.](https://twitter.com/odysseyteam_) for Chimera

