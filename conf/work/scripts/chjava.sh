#/bin/bash

JAVA7_DIR="java-7-openjdk-amd64"
JAVA6_45_DIR="jdk1.6.0_45"

version="$1"


rm -f /etc/alternatives/java*

if [ "x$version" == "x7" ] ; then
	ln -s  	/usr/lib/jvm/java-7-openjdk-amd64/jre/bin/java				/etc/alternatives/java          
	ln -s   /usr/lib/jvm/java-7-openjdk-amd64/jre/man/man1/java.1.gz                /etc/alternatives/java.1.gz     
	ln -s   /usr/lib/jvm/java-7-openjdk-amd64/bin/javac                            /etc/alternatives/javac         
	ln -s   /usr/lib/jvm/java-7-openjdk-amd64/man/man1/javac.1.gz                   /etc/alternatives/javac.1.gz    
	ln -s   /usr/lib/jvm/java-7-openjdk-amd64/bin/javadoc                          /etc/alternatives/javadoc       
	ln -s  	/usr/lib/jvm/java-7-openjdk-amd64/man/man1/javadoc.1.gz                 /etc/alternatives/javadoc.1.gz  
	ln -s   /usr/lib/jvm/java-7-openjdk-amd64/bin/javah                            /etc/alternatives/javah         
	ln -s   /usr/lib/jvm/java-7-openjdk-amd64/man/man1/javah.1.gz                   /etc/alternatives/javah.1.gz    
	ln -s   /usr/lib/jvm/java-7-openjdk-amd64/bin/javap                            /etc/alternatives/javap         
	ln -s   /usr/lib/jvm/java-7-openjdk-amd64/man/man1/javap.1.gz                   /etc/alternatives/javap.1.gz    
	ln -s   /usr/lib/jvm/java-7-openjdk-amd64/jre/bin/javaws                       /etc/alternatives/javaws        
	ln -s 	/usr/lib/jvm/java-7-openjdk-amd64/jre/man/man1/javaws.1.gz              /etc/alternatives/javaws.1.gz   
elif [ "x$version" == "x6" ]; then
	ln -s  	/usr/lib/jvm/jdk1.6.0_45/bin/java				/etc/alternatives/java          
	ln -s   /usr/lib/jvm/jdk1.6.0_45/man/man1/java.1                /etc/alternatives/java.1.gz     
	ln -s   /usr/lib/jvm/jdk1.6.0_45/bin/javac                            /etc/alternatives/javac         
	ln -s   /usr/lib/jvm/jdk1.6.0_45/man/man1/javac.1                   /etc/alternatives/javac.1.gz    
	ln -s   /usr/lib/jvm/jdk1.6.0_45/bin/javadoc                          /etc/alternatives/javadoc       
	ln -s  	/usr/lib/jvm/jdk1.6.0_45/man/man1/javadoc.1                 /etc/alternatives/javadoc.1.gz  
	ln -s   /usr/lib/jvm/jdk1.6.0_45/bin/javah                            /etc/alternatives/javah         
	ln -s   /usr/lib/jvm/jdk1.6.0_45/man/man1/javah.1                   /etc/alternatives/javah.1.gz    
	ln -s   /usr/lib/jvm/jdk1.6.0_45/bin/javap                            /etc/alternatives/javap         
	ln -s   /usr/lib/jvm/jdk1.6.0_45/man/man1/javap.1                   /etc/alternatives/javap.1.gz    
	ln -s   /usr/lib/jvm/jdk1.6.0_45/bin/javaws                       /etc/alternatives/javaws        
	ln -s 	/usr/lib/jvm/jdk1.6.0_45/man/man1/javaws.1              /etc/alternatives/javaws.1.gz   
else
	echo "need a version number"
fi
