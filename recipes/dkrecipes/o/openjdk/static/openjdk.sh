# Begin /etc/profile.d/openjdk.sh

# Set JAVA_HOME directory
export JAVA_HOME=/opt/jdk

# Add Java binaries to PATH manually
export PATH="$PATH:$JAVA_HOME/bin"

# Auto Java CLASSPATH: Copy jar files to, or create symlinks in, the
# /usr/share/java directory.

AUTO_CLASSPATH_DIR=/usr/share/java

# Start CLASSPATH with "."
export CLASSPATH=.

# Add all directories under /usr/share/java to CLASSPATH
for dir in $(find "${AUTO_CLASSPATH_DIR}" -type d 2>/dev/null); do
    CLASSPATH="$CLASSPATH:$dir"
done

# Add all .jar files under /usr/share/java to CLASSPATH
for jar in $(find "${AUTO_CLASSPATH_DIR}" -name "*.jar" 2>/dev/null); do
    CLASSPATH="$CLASSPATH:$jar"
done

export CLASSPATH

# By default, Java creates several files in a directory named
# /tmp/hsperfdata_[username]. This directory contains files that are used for
# performance monitoring and profiling, but aren't normally needed on a BLFS
# system. This environment variable disables that feature.
export _JAVA_OPTIONS="-XX:-UsePerfData"

unset AUTO_CLASSPATH_DIR dir jar

# End /etc/profile.d/openjdk.sh
