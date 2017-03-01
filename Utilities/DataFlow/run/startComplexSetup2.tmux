#!/usr/bin/env tmux source-file
# simple start script to launch a more complex setup
# with 2 data publishers (inside subframebuilder) + 2 attached flpSenders + 4 EPNS
# Prerequisites:
#  - expects the configuration file to be in the working directory
#  - O2 bin and lib set n the shell environment

# it would be nice having a script that generates the configuration file for N FLP and M EPNS
# Start one HBSampler device
set-option remain-on-exit on
new-window 'heartbeatSampler'
split-window alienv setenv O2/latest -c watch ps 
#heartbeatSampler --id heartbeatSampler --mq-config confComplexSetup2.json --out-chan-name output

# Data publishers
new-window 'publisher'
split-window DataPublisherDevice --id DataPublisherDeviceTPC --mq-config confComplexSetup2.json --in-chan-name input --out-chan-name output
split-window DataPublisherDevice --id DataPublisherDeviceITS --mq-config confComplexSetup2.json --in-chan-name input --out-chan-name output


new-window 'subframe builders'
# this is the subtimeframe publisher for TPC
split-window SubframeBuilderDevice --id subframeBuilderTPC --mq-config confComplexSetup2.json --detector TPC
# this is the subtimeframe publisher for ITS
split-window SubframeBuilderDevice --id subframeBuilderITS --mq-config confComplexSetup2.json --detector ITS

new-window 'flp'
# this is the flp for TPC
split-window flpSender --id flpSenderTPC --mq-config confComplexSetup2.json --in-chan-name input --out-chan-name output --num-epns 4 --flp-index 0
# this is the flp for ITS
split-window flpSender --id flpSenderITS --mq-config confComplexSetup2.json --in-chan-name input --out-chan-name output --num-epns 4 --flp-index 1

new-window 'epn'
# we have 4 epn and 2 flps
split-window epnReceiver --id epnReceiver1 --mq-config confComplexSetup2.json --in-chan-name input --out-chan-name output --num-flps 2
split-window epnReceiver --id epnReceiver2 --mq-config confComplexSetup2.json --in-chan-name input --out-chan-name output --num-flps 2
split-window epnReceiver --id epnReceiver3 --mq-config confComplexSetup2.json --in-chan-name input --out-chan-name output --num-flps 2
split-window epnReceiver --id epnReceiver4 --mq-config confComplexSetup2.json --in-chan-name input --out-chan-name output --num-flps 2

new-window 'validator'
# consumer and validator of the full EPN time frame
split-window TimeframeValidatorDevice --id timeframeValidator --mq-config confComplexSetup2.json --input-channel-name input
