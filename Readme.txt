ReadMe
ระบบวัดการใช้พลังงานไฟฟ้าของอุปกรณ์ (Smart Energy Plug)

สิ่งที่ต้องมี
VScode	https://code.visualstudio.com/download
MQTTX	https://mqttx.app/downloads(หากต้องการดูการส่งข้อมูล)
ขั้นตอนการsetไฟล์
1.ให้เปิดไฟล์Documents
2.นำโฟลเดอร์PlatformIOไปไว้
3.จากนั้นให้กดเข้าไปและนำไฟล์มาใส่ และแตกไฟล์จะได้โฟลเดอร์ชื่อiotและจะมีไฟล์ต่างๆด้านใน
ขั้นที่1
1.เปิดvscodeโดยให้โหลดExtensions(Wokwi Simulator,PlatformIO IDE) จากนั้นให้กดfileและกดopen Folder และเลือกโฟลเดอร์iot
2.กดF1และเลือกPlatformlO:Open PlatformIO Core CLI โดยมันจะเป็นterminalของตัวPlatformlO(หากมันเด้งให้reloadให้กดreloadด้านขวาล่าง)
3.ให้ดูpathให้ตรงตามชื่อโปรเจคเช่น อยู่C:\Users\uSeR\Documents\PlatformIO\iot ให้ใส่คำสั่ง-cd "C:\Users\uSeR\Documents\PlatformIO\iot"
4.ใส่คำสั่ง-pio run และรอจนกว่าจะเสร็จ
5.กดF1หรือclt+shift+PและเลือกWokwi Start Simulator(หากกดไม่ได้ไปข้อ6)
6.หากเคยทำการstart simulateไปแล้วให้ไปไฟล์diagram.json
----------------------------------------------------------------
|ถ้าหากขึ้นErrorขวาให้ทำตามนี้|
1.หลั้งครึ่งErrorให้คลิกที่Get your License Keyจะขึ้นหน้าเว็ปwokwi
2.ให้loginจากนั้นให้กดGET YOUR LICENSE และจะมีแถบบนเด้งกดopen vscode
3.และกดF1หรือclt+shift+PและเลือกWokwi Start Simulatorอีกครั้ง
 ----------------------------------------------------------------
6.จะแสดงEmulatorโดยจะแสดงEsp32และกดปุ่มเขียว
7.รอจนกว่าจะเชื่อมMQTT
ขั้นตอน2(หากต้องการดูการส่งjson)
1.เปิดMQTTX
2.ให้กด+Connection
3.ให้ใส่ชื่อและแก้Client IDเป็นmqttx_mornitor และกดconnect
4.ให้เพิ่มsubscription และโดยใส่topicชื่อpzem/esp32/# และกดConfirm จะแสดงข้อมูลจากvscode
ขั้นตอน3Dashboard
1.ให้เปิดไฟล์html
2.กดปุ่มเชื่อมต่อฝั่งซ้ายจะแสดงสถานะการเชื่อมต่อ
-----------------------------------------------------------------
จากนั้นจะเสร็จสมบูรณ์โดยจะมีกราฟ อุปกรณ์ต่างๆที่แสดงค่าใช้จ้าย และหน่อยไฟต่างๆ
