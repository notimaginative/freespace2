FROM debian:bookworm-slim

ENV DEBIAN_FRONTEND=noninteractive

# set up base install stuff and deps for SDL, etc.
RUN apt -yq update && apt -yq upgrade && \
	apt -yq install build-essential git make ninja-build pkg-config \
		ca-certificates gpg wget curl sudo vim cmake file && \
	apt -yq install gnome-desktop-testing libasound2-dev libpulse-dev \
		libaudio-dev libjack-dev libsndio-dev libx11-dev libxext-dev \
		libxrandr-dev libxcursor-dev libxfixes-dev libxi-dev libxss-dev libxtst-dev \
		libxkbcommon-dev libdrm-dev libgbm-dev libgl1-mesa-dev libgles2-mesa-dev \
		libegl1-mesa-dev libdbus-1-dev libibus-1.0-dev libudev-dev \
		libpipewire-0.3-dev libwayland-dev libdecor-0-dev liburing-dev \
		libunwind-dev libusb-1.0-0-dev

