# Istall Raspbian on the Raspberry PI

- Baixe a imagem do sistema RASPBIAN STRETCH WITH DESKTOP no site do raspberry (https://www.raspberrypi.org/downloads/raspbian/)
Obs: Nao baixe o arquivo LITE pois este possui apenas interface por linha de comando.

- Será necessário identificar o nome do dispositivo do cartão SD. Para isto, antes de inserir o cartão de memória execute o seguinte comando para ver as unidades de disco presentes.

```bash
 $ sudo fdisk -l
 ou
 $ ls -la /dev/sd*
```
- Insira o cartão e execute novamente o comando de forma a identificar o nome do cartao de SD que irá aparecer. Será algo do tipo /dev/sd...

```bash
 $ sudo fdisk -l
```

- Caso o cartão SD apareça como por exemplo /dev/sde1 ou /dev/sde2, eliminar o numero e considerar apenas /dev/sde 

- Execute o seguinte comando para copiar a imagem do Raspibian para o cartão SD fazendo as alterações de caminho necessárias.

```bash
 $ sudo dd if=/home/usr/Downloads/2018-04-18-raspbian-stretch.img of=/dev/sd...
```

- O cartão esta formatado e pode ser inserido no Raspberry para utilização.


# Enable the IMU

- Não execute upgrade

```bash
 $ sudo apt-get update
 $ sudo apt-get install i2c-tools libi2c-dev python-smbus
 $ sudo raspi-config
```
- Acesse e habilite a camera:
->Interfacing Options
       ->Camera

Para testar: O comando raspistill grava uma imgem e raspivid grava um video 

```bash
 $ raspistill -v -o test.jpg
 $ raspivid -o teste.h264 -t 10000
```


# Install Dependencies anf Download the pi_imu file from git

```bash
 $ sudo apt-get install subversion
 $ svn checkout https://github.com/LCAD-UFES/carmen_lcad/trunk/src/pi_imu
```


# Compile and test the pi_imu module on the Raspberry PI

```bash
 $ cd pi_camera/raspicam && mkdir build && cd build && cmake ..
 $ make
 $ sudo make install
 $ cd ../.. && mkdir build && cd build && cmake ..
 $ make
 $ ./pi_camera_test
```

 The pi_camera_test program will run the camera and display the image captured using OpenCv


# Configure an Static IP to the Raspberry PI on IARA's network
 
 Start by editing the dhcpcd.conf file
 
```bash
 $ sudo nano /etc/dhcpcd.conf
```

 Add at the end of the file the following configuration:
 eth0 = wired connection
 For the first pi_camera we used the IP adress 192.168.0.15/24
 Replace the 15 with the IP number desired
 Make sure you leave the /24 at the end of the adress
 
```ini
 interface eth0

 static ip_address=192.168.0.15/24
 static routers=192.168.0.1
 static domain_name_servers=192.168.0.1
``` 

 To exit the editor, press ctrl+x
 To save your changes press the letter “Y” then hit enter
 
 Reboot the Raspberry PI
 
```bash
 $ sudo reboot
```

 You can double check by typing:
 
```bash
 $ ifconfig
```

 The eth0 inet addr must be 192.168.0.15
