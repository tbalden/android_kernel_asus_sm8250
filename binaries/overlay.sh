#!/system/bin/sh
#umount /system/etc/hosts

rm /dev/hosts_k
unzip /dev/hosts_k.zip -d /dev/ -o

touch /dev/hosts_k
chmod 644 /dev/hosts_k
chcon u:object_r:system_file:s0 /dev/hosts_k

#touch /storage/emulated/0/hosts_k
#chcon u:object_r:system_file:s0 /storage/emulated/0/hosts_k

# This breaks system integrity ---vvv
#/system/bin/mount -o ro,bind /dev/hosts_k /system/etc/hosts
