# Img file
If you want to create an image file to load your distro in instead of installing 
to real hardware start here. Otherwise skip to Installation.


First we must create an image file. (I'm doing 40G as my dkrecipes currently is 
~35G as a complete build). If you don't know what size to make the disk.img run 
the following command.

```sh
du -h mnt/lfs
```

From here I would make it just a little bit bigger to make sure there is enough 
room to grow. However, you cannot make it smaller than the minimal size that du
shows.

status=progress is optionally.
```sh
dd if=/dev/zero of=lfs.img bs=1M count=40960 status=progress
```
or if your filesystem supports it. (ext3/4, xfs, btrfs, ntfs) Some sites list
ext2, but from expirence this is not supported.
```sh
truncate -s 40G lfs.img
```

Now we need to mount the  img file.
```sh
sudo losetup -fP lfs.img
losetup -a
```

losetup -a should give you an output like:
/dev/loop0: []: (/home/dakota/lfs.img)

format your drive. I'm using cfdisk. However, you can use fdisk or any other 
tool. For simplicity the drive should be MBR (dos) with one  ext[2-4] partition.
```sh
sudo cfdisk /dev/loop0
```

Format the file system
```sh
sudo mkfs.ext4 /dev/loop0p1
```

Create a place to mount our image, in the following installation section I use 
/mnt/usb as an example location. Replace it with this one or wherever you
mounted your image at.
```sh
sudo mkdir -vp /mnt/lfs-image
sudo mount /dev/loop0p1 /mnt/lfs-image
```

Once done Continue on with Installation as if it was on a real drive.


# Installation
If you did not build in place. Either by mounting your drive to mnt/lfs or 
changing the relative build path to where your new drive is mounted you will 
need to transfer your files over. In this example we mounted our newroot 
partition to /mnt/usb, change it to wherever you mounted your new root fs.

As this command is set up it needs to be executed in the root of the mylfs 
directory.

```sh
sudo rsync -aAXHv --delete --info=progress2 \
 --exclude=/dev/* \
 --exclude=/proc/* \
 --exclude=/sys/* \
 --exclude=/run/* \
 mnt/lfs/ /mnt/usb/
```

Then you need to bind mount {/dev/{pts,shm},sys, run, proc} at your mount point.

```sh
mount --bind /dev  /mnt/usb/dev
mount --bind /proc /mnt/usb/proc
mount --bind /sys  /mnt/usb/sys
mount --bind /run  /mnt/usb/run
```

if /dev/pts exist it should be mounted as well.

```sh
mount --bind /dev/pts  /mnt/usb/dev/pts
```

Once done chroot inside.
```sh
chroot /mnt/usb /usr/bin/bash --login
```

When done make sure to unmount your bind mounts


# configure
fstab & hosts need to be configured inside. This is a good time to make any last
 minute configuration to other files as well. 

Brave souls can try genfstab, however, I can't guarantee that it will work. 
therwise refer to the LFS book for setting up this file. This modified version 
also allows for a -C to be used with -P to allow for a clean style LFS fstab 
setup. --- Experimental

```sh
genfstab -LP > /etc/fstab
```

Make sure to set a root password.

NOTE: This should be patched. However, when I first booted I could not set the 
root password. Doing this seems to have fixed that issue.


# grub2
In our Chroot we need to install grub2 to our drive. My drive is /dev/sdc make 
sure you properly identify which drive you are actually using.

If you want to install legacy mode for qemu or older hardware and your system 
supports UEFI you need to pass --target=i386-pc

With a gpt formatted disk, to install legacy mode you need a 1-2 mb unformatted 
space at the beginning of the disk that you mark as bios-grub. This will also 
allow you to make a medium that can boot both in legacy and UEFI as long as the 
kernel supports it. To do this after you install grub to the root of the drive 
you would then install grub normally for an efi setup with a fat32 (vfat) fs as 
your esp/boot/efi depending on the setup that is placed right after the 1-2 mb 
unformatted space for legacy grub. -- Make sure that your mount your EFI 
partition inside your chroot before installing grub if your using a efi setup.

If you're in the mounted image this should look like /dev/loop0
```sh
grub-install /dev/sdc
```
### Basic config
example grub2 config from the book to be placed in  /boot/grub/grub.cfg
---
# Begin /boot/grub/grub.cfg
set default=0
set timeout=5


insmod part_gpt
insmod ext2
set root=(hd0,2)
set gfxpayload=1024x768x32


menuentry "GNU/Linux, Linux 6.13.9-lfs-12.3" {
       linux   /boot/vmlinuz-6.13.9-lfs-12.3 root=/dev/sda2 ro
}
---

### Config with PARTUUID (recommended)
Grub config from the bash script MyLFS that I based this off.
You can get the Part UUID from running blkid and passing in your drive with it's
partition id.

It should be noted that /dev/sdc is the drive itself, sdc4 is the fourth partion 
on the drive. (usually)
```sh
blkid /dev/sdc4
```

In it's output you'll see PARTUUID="". What ever is in the string is your part 
uuid and would go in the grub config. (Best success using this)

Make sure your partition is labled with LFSROOT or update the --label with your 
root fs label
---
# Begin /boot/grub/grub.cfg
set default=0
set timeout=5

insmod ext2

menuentry "GNU/Linux, Linux 6.13.9-lfs-12.3 nomodeset"{
  search --no-floppy --label LFSROOT --set=root
  linux   /boot/vmlinuz-6.13.9-lfs-12.3 rootwait root=PARTUUID=ecb913d1-02 ro net.ifnames=0 biosdevname=0 nomodeset
}

menuentry "GNU/Linux, Linux 6.13.9-lfs-12.3" {
  search --no-floppy --label LFSROOT --set=root
  linux   /boot/vmlinuz-6.13.9-lfs-12.3 rootwait root=PARTUUID=ecb913d1-02 ro net.ifnames=0 biosdevname=0
}


---

if you build dkrecipes you should have grub-mkconfig command.  I like to use 
UUIDs instead of /dev/sdX notation.


cat > /etc/default/grub << "EOF"
GRUB_DISABLE_LINUX_UUID="false"
GRUB_TIMEOUT=5
GRUB_DISTRIBUTOR="GNU/Linux"
GRUB_CMDLINE_LINUX_DEFAULT=""
GRUB_CMDLINE_LINUX=""
EOF


```sh
grub-mkconfig -o /boot/grub/grub.cfg
```


Now we need to unmount the bind mounts. If you also mounted dev/pts that needs 
to be unmounted before

/mnt/usb/dev is.
```sh
umount -l /mnt/usb/dev
umount -l /mnt/usb/proc
umount -l /mnt/usb/sys
umount -l /mnt/usb/run
umount -l /mnt/usb/
sync
```


# Trying the system in qemu


To try this in qemu you must also umount your new root fs.
```sh
umount /mnt/usb
```

if you were working with the image file also run
```sh
losetup -d /dev/loop0
```
Remember my drive was /dev/sdc. Make sure to pass in the right drive or lfs.img.

When creating the grub config, the system /dev/ is mounted. However, when 
booting the drive directly in qemu like done below it becomes the only drive. 
Thus I had to edit the grub config and change /dev/sdc4 to /dev/sda4. eg. 
linux /boot/vmlinuz-6.13.9-lfs-12.3 root=/dev/sda3 ro


```sh
qemu-system-x86_64 -m 2048 -enable-kvm -cpu host -hda /dev/sdc -boot order=d -vga std
```

If you hardware supports it you can add in place of "-vga std"
```sh
qemu-system-x86_64 -m 2048 -enable-kvm -cpu host -hda /dev/sdc -boot order=d  -device virtio-vga-gl -display gtk,gl=on
```