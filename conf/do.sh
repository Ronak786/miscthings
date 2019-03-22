#!/bin/bash
update-rc.d ntp disable
systemctl disable ntp
systemctl disable systemd-timesyncd
systemctl disable carbon-cache
systemctl disable apache2
systemctl disable collectd
systemctl disable grafana-server
systemctl disable glusterfs-server
systemctl disable petasan-mount-sharedfs
systemctl disable petasan-cluster-leader
systemctl disable petasan-start-osds

ln -s /opt/petasan/scripts/cron-1h.py /etc/cron.hourly/cron-1h.py
ln -s /opt/petasan/scripts/cron-1d.py /etc/cron.daily/cron-1d.py

systemctl enable  petasan-deploy.service
systemctl enable  petasan-start-services.service


# SSH configuration 
update-rc.d ssh defaults 
# server
sed -i -- 's/PermitRootLogin prohibit-password/PermitRootLogin yes/g'  /etc/ssh/sshd_config
echo "UseDNS no" >>  /etc/ssh/sshd_config
echo "GSSAPIAuthentication no" >>  /etc/ssh/sshd_config
# client
sed -i -- 's/GSSAPIAuthentication yes/GSSAPIAuthentication no/g'  /etc/ssh/ssh_config
echo "ServerAliveInterval 5 " >>  /etc/ssh/ssh_config
echo "ServerAliveCountMax 2 " >>  /etc/ssh/ssh_config



# ----- New Installation  ----------

useradd -d /home/ceph -m ceph

mkdir -p /opt/petasan/config/shared
mkdir -p /opt/petasan/config/gfs-brick

# create sym links

mkdir -p /opt/petasan/config/etc/ceph
rm -rf /etc/ceph
ln -s /opt/petasan/config/etc/ceph /etc/ceph

mkdir -p /opt/petasan/config/etc
cp /etc/hosts /opt/petasan/config/etc
rm -f /etc/hosts
ln -s /opt/petasan/config/etc/hosts /etc/hosts

cp -r /etc/ssh /opt/petasan/config/etc
rm -rf /etc/ssh
ln -s /opt/petasan/config/etc/ssh /etc/ssh

if [ -d /root/.ssh  ]; then
  mkdir -p /opt/petasan/config/root
  cp -r  /root/.ssh /opt/petasan/config/root
  rm -rf /root/.ssh
else
  mkdir -p /opt/petasan/config/root/.ssh
fi
ln -s /opt/petasan/config/root/.ssh /root/.ssh

mkdir -p /opt/petasan/config/etc/network
cp -f /etc/network/interfaces /opt/petasan/config/etc/network
rm -f /etc/network/interfaces
ln -s /opt/petasan/config/etc/network/interfaces  /etc/network/interfaces

cp /etc/resolv.conf /opt/petasan/config/etc
rm -f /etc/resolv.conf
ln -s /opt/petasan/config/etc/resolv.conf /etc/resolv.conf

cp /etc/ntp.conf /opt/petasan/config/etc
rm -f /etc/ntp.conf
ln -s /opt/petasan/config/etc/ntp.conf /etc/ntp.conf

readlink /etc/localtime  > /opt/petasan/config/etc/tz

mkdir -p /opt/petasan/config/var/lib
cp -rf /var/lib/glusterd  /opt/petasan/config/var/lib
rm -rf /var/lib/glusterd
ln -s /opt/petasan/config/var/lib/glusterd /var/lib/glusterd


# sym links do not work for these, just keep a copy in config

cp  /etc/hostname /opt/petasan/config/etc/hostname
# rm -f /etc/hostname
# ln -s /opt/petasan/config/etc/hostname /etc/hostname

cp /etc/passwd /opt/petasan/config/etc
# rm -f /etc/passwd
# ln -s /opt/petasan/config/etc/passwd /etc/passwd

cp /etc/shadow /opt/petasan/config/etc
# rm -f /etc/shadow
# ln -s /opt/petasan/config/etc/shadow /etc/shadow
