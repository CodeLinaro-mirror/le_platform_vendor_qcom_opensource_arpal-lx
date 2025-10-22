if BUILDSYSTEM_OPENWRT
SUBDIRS = SpeakerMic Speaker HeadsetMic Headphone HandsetMic Handset DummyDev
else
if BUILDSYSTEM_TELEMATICS
SUBDIRS = SpeakerMic Speaker HeadsetMic Headphone HandsetMic Handset Bluetooth
else
SUBDIRS = USBAudio UltrasoundDevice SpeakerMic Speaker HeadsetVaMic HeadsetMic Headphone HapticsDev HandsetVaMic HandsetMic Handset FMDevice ExtEC ECRefDevice DummyDev DisplayPort Bluetooth RTProxy
endif
endif