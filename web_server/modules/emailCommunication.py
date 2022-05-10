import smtplib, time
from modules import thingspeak

emailSleep = 86400 # u sekundama (86400 = 1 dan)

def sendEmail():
    global emailSleep

    while True:
        time.sleep(emailSleep) # vreme u sekundama

        EMAIL_ADDRESS = "" #input("Unesite email: ")
        PASSWORD = "" #input("Unesite lozinku za email: ")
        TO_ADDRESS = ""

        if EMAIL_ADDRESS != "" and PASSWORD != "" and TO_ADDRESS != "":
            
            server = smtplib.SMTP('smtp.gmail.com', 587)
            server.starttls()
            resCode = server.login(EMAIL_ADDRESS, PASSWORD)

            thingSpeakResponse = thingspeak.readDataFromThingSpeak()
            thingSpeakResponse = thingSpeakResponse["feeds"]
            totalItems = len(thingSpeakResponse)

            avgTemp = 0
            avgLight = 0
            garageOpened = 0
            relaySwitched = 0

            for item in thingSpeakResponse:
                avgTemp = avgTemp + float(item["field1"])
                avgLight = avgLight + float(item["field2"])
                garageOpened = garageOpened + int(item["field3"])
                relaySwitched = relaySwitched + int(item["field4"])
                
            avgTemp = avgTemp/totalItems
            avgTemp = round(avgTemp, 2)
            avgLight = avgLight/totalItems
            avgLight = round(avgLight, 2)

            subject = "Dnevni izvestaj"
            body = "Automatski poslat mejl sa dnevnim izvestajem. \n\n" + "Prosecna temperatura: " + str(avgTemp) + " C.\n" + "Prosecno osvetljenje: " + str(avgLight) + "%.\n" + "Ukupan broj otvaranja garaznih vrata: " + str(garageOpened) + ".\n" + "Ukupan broj promene stanja releja: " + str(relaySwitched) + ".\n\n" + "Pozdrav. Tvoj Arduino."
            fullEmail = "Subject: {}\n\n{}".format(subject, body)

            resCode = server.sendmail(from_addr=EMAIL_ADDRESS, to_addrs=TO_ADDRESS, msg=fullEmail)

            server.quit()