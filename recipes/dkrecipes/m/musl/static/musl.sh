# /etc/profile.d/musl.sh

# Add ZVM managed binaries (Zig versions) to user's PATH
if [ -d "/musl/bin" ]; then
    export PATH="$PATH:/musl/bin"
fi