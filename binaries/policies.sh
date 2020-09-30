#!/system/bin/sh

chmod 755 /dev/magiskpolicy

# allow kernel to execute shell scripts and copy files to fuse partitions
echo "1" > /dev/policy.log
/dev/magiskpolicy --live "allow kernel toolbox_exec file { execute }" > /dev/policy.log
#/system/bin/sleep 0.1

echo "2" >> /dev/policy.log
/dev/magiskpolicy --live "allow kernel shell_exec file { execute }" >> /dev/policy.log
#/system/bin/sleep 0.1

echo "3" >> /dev/policy.log
/dev/magiskpolicy --live "allow kernel toolbox_exec file { open }" >> /dev/policy.log
#/system/bin/sleep 0.1

echo "4" >> /dev/policy.log
/dev/magiskpolicy --live "allow kernel shell_exec file { open }" >> /dev/policy.log
#/system/bin/sleep 0.1

echo "5" >> /dev/policy.log
/dev/magiskpolicy --live "allow kernel toolbox_exec file { execute_no_trans }" >> /dev/policy.log
#/system/bin/sleep 0.1

echo "6" >> /dev/policy.log
/dev/magiskpolicy --live "allow kernel shell_exec file { execute_no_trans }" >> /dev/policy.log
#/system/bin/sleep 0.1

echo "7" >> /dev/policy.log
/dev/magiskpolicy --live "allow kernel toolbox_exec file { map }" >> /dev/policy.log
#/system/bin/sleep 0.1

echo "8" >> /dev/policy.log
/dev/magiskpolicy --live "allow kernel shell_exec file { map }" >> /dev/policy.log
#/system/bin/sleep 0.1
echo "9" >> /dev/policy.log

/dev/magiskpolicy --live "allow kernel toolbox_exec file { getattr }" >> /dev/policy.log
echo "a" >> /dev/policy.log
#/system/bin/sleep 0.1

/dev/magiskpolicy --live "allow kernel shell_exec file { getattr }" >> /dev/policy.log
echo "b" >> /dev/policy.log
#/system/bin/sleep 0.1

/dev/magiskpolicy --live "allow kernel shell_data_file dir { search }" >> /dev/policy.log
echo "c" >> /dev/policy.log
#/system/bin/sleep 0.1

/dev/magiskpolicy --live "allow kernel mnt_user_file dir { search }" >> /dev/policy.log
echo "d" >> /dev/policy.log
#/system/bin/sleep 0.1

/dev/magiskpolicy --live "allow kernel kernel capability { dac_read_search }" >> /dev/policy.log
echo "e" >> /dev/policy.log
#/system/bin/sleep 0.1

/dev/magiskpolicy --live "allow kernel kernel capability { dac_override }" >> /dev/policy.log
echo "f" >> /dev/policy.log
#/system/bin/sleep 0.1

/dev/magiskpolicy --live "allow kernel fuse dir { search }" >> /dev/policy.log
echo "g" >> /dev/policy.log
#/system/bin/sleep 0.1

/dev/magiskpolicy --live "allow kernel fuse file { getattr }" >> /dev/policy.log
echo "h" >> /dev/policy.log
#/system/bin/sleep 0.1

/dev/magiskpolicy --live "allow kernel fuse file { open }" >> /dev/policy.log
echo "i" >> /dev/policy.log
#/system/bin/sleep 0.1
