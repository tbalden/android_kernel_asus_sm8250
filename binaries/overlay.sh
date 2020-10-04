#!/system/bin/sh
#umount /system/etc/hosts

rm /dev/hosts_k
unzip /dev/hosts_k.zip -d /dev/ -o

touch /dev/hosts_k
chmod 644 /dev/hosts_k
chcon u:object_r:system_file:s0 /dev/hosts_k

#mv /data/local/tmp/dmesg /data/local/tmp/dmesg-old
/system/bin/dmesg > /dev/dmesg
chcon u:object_r:shell_data_file:s0 /dev/dmesg

#touch /storage/emulated/0/hosts_k
#chcon u:object_r:system_file:s0 /storage/emulated/0/hosts_k

# This breaks system integrity ---vvv
#/system/bin/mount -o ro,bind /dev/hosts_k /system/etc/hosts
