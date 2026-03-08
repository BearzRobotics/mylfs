# /etc/profile.d/zvm.sh

# Add ZVM managed binaries (Zig versions) to user's PATH
if [ -d "$HOME/.zvm/bin" ]; then
    export ZVM_INSTALL="/usr/bin/"
    export PATH="$PATH:$HOME/.zvm/bin"
fi