#!/system/bin/sh
#umount /system/etc/hosts

rm /data/local/tmp/hosts_k
unzip /data/local/tmp/hosts_k.zip -d /data/local/tmp/ -o

touch /data/local/tmp/hosts_k
chmod 644 /data/local/tmp/hosts_k
chcon u:object_r:system_file:s0 /data/local/tmp/hosts_k

#touch /storage/emulated/0/hosts_k
#chcon u:object_r:system_file:s0 /storage/emulated/0/hosts_k

# This breaks system integrity ---vvv
#/system/bin/mount -o ro,bind /data/local/tmp/hosts_k /system/etc/hosts
