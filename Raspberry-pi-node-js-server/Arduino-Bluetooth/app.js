
'use strict';
//ENCRYPTION ALGORITHM RETRIEVED FROM: https://gist.github.com/siwalikm/8311cf0a287b98ef67c73c1b03b47154
const crypto = require('crypto');
const ENC_KEY = "bf3c199c2470cb477d907b1e0917c17b"; // set random encryption key
const IV = "5183666c72eec9e4"; // set random initialisation vector
// ENC_KEY and IV can be generated as crypto.randomBytes(32).toString('hex');

//Encrypt function
var encrypt = ((val) => {
  let cipher = crypto.createCipheriv('aes-256-cbc', ENC_KEY, IV);
  let encrypted = cipher.update(val, 'ASCII', 'base64');
  encrypted += cipher.final('base64');
  return encrypted;
});

//decrypt function
var decrypt = ((encrypted) => {
  let decipher = crypto.createDecipheriv('aes-256-cbc', ENC_KEY, IV);
  let decrypted = decipher.update(encrypted, 'base64', 'ASCII');
  return (decrypted + decipher.final('ASCII'));
});

//USAGE OF ENCRYPT-DECRYPT EXAMPLE
//const phrase = "who let the dogs out";
//encrypted_key = encrypt(phrase);
//original_phrase = decrypt(encrypted_key);

// GOOGLE DATASTORE AND AUTOML DECLARATIONS

// By default, the client will authenticate using the service account file
// specified by the GOOGLE_APPLICATION_CREDENTIALS environment variable and use
// the project specified by the GOOGLE_CLOUD_PROJECT environment variable. See
// https://github.com/GoogleCloudPlatform/google-cloud-node/blob/master/docs/authentication.md
// These environment variables are set automatically on Google App Engine
var {Datastore} = require('@google-cloud/datastore');

// Instantiate a datastore client
var datastore = new Datastore();
//Get the automl libraries
var automl = require('@google-cloud/automl')

//Set client options to europe
var clientOptions = {apiEndpoint: 'eu-automl.googleapis.com'}

var client = new automl.v1beta1.PredictionServiceClient(clientOptions)

//Set project id
var projectID = 'christos901129test'
//Set computed Region
var computeRegion = 'eu'
//Set modelID
var modelId = 'TBL4353116067946561536'

//Get the full path of the model.
var modelFullId = client.modelPath(projectID, computeRegion, modelId)


/**
 * Insert a visit record into the database.
 *
 * @param {object} visit The visit record to insert.
 */
 
 var insertVisit = visit => {
  return datastore.save({
    key: datastore.key('visit'),
    data: visit,
  });
};

//END OF GOOGLE DATASTORE AND AUTOML DECLARATIONS


//STEP 1: Get Adapter
const {createBluetooth} = require('node-ble')
const {bluetooth, destroy} = createBluetooth()

//Function for finding arrays
Array.prototype.contains = function (needle) {
	for (var i in this){
		if (this[i] == needle) return true
		}
		return false
	}

//Main function
async function main () 
{
	const timeoutId = setTimeout(() => {
		console.error('Watchdog timer timeout, program terminates')
		process.exit(1)
		}, 60* 1000)

		
		timeoutId.unref()
//STEP 1: Get Adapter

console.log('wait for bluetooth default adapter')
const adapter = await bluetooth.defaultAdapter()

//STEP 2: Start discovering

if (! await adapter.isDiscovering())
  await adapter.startDiscovery()

console.log('bluetooth is discovering')

console.log('list devices')
console.log(await adapter.devices())
console.log('Find arduino device ')
console.log('Find device TemperatureMonitor01 F0:1A:89:DE:14:65')

//While our adapter is not finding arduino device
var devicelist = await adapter.devices()
while(!devicelist.contains('F0:1A:89:DE:14:65') )
{
	//Keep scanning until device is found
	console.log('list devices')
	console.log(devicelist)
	devicelist = await adapter.devices()
	console.log('Find arduino device ')
	console.log('Find device TemperatureMonitor01 F0:1A:89:DE:14:65')
	//wait 1 second
	await new Promise(resolve => setTimeout(resolve, 1000))
}

//STEP 3: Get a device, Connect and Get GATT Server
console.log('device inside the string array')
const device = await adapter.waitDevice('F0:1A:89:DE:14:65')
await device.connect()
console.log('connect to the device')
const gattServer = await device.gatt()
console.log('wait 1 sec')
	while (1){
//Print all the primary characteristics
console.log('gattServer characteristics')
console.log(await gattServer.services())

//STEP 4a: Read and write a characteristic.
//NOTE: Only one device can connect to the arduino, reason maybe the while connect (check it later or implement it that only
//raspberry pi connects to it

const service1 = await gattServer.getPrimaryService('9a48ecba-2e92-082f-c079-9e75aae428b1')
const characteristic1 = await service1.getCharacteristic('2d2f88c4-f244-5a80-21f1-ee0224e80658')
//await characteristic1.writeValue(Buffer.from("Hello world"))
const buffer = await characteristic1.readValue()
console.log('buffer is:')
//print the string
//const data_decrypted = decrypt(String.fromCharCode.apply(null, buffer))
console.log(String.fromCharCode.apply(null, buffer))

//STEP 4b: Subscribe to a characteristic.
/*
const service2 = await gattServer.getPrimaryService('9A48ECBA-2E92-082F-C079-9E75AAE428B1')
const characteristic2 = await service2.getCharacteristic('0x2902')//('2D2F88C4-F244-5A80-21F1-EE0224E80658')
await characteristic2.startNotifications()
characteristic2.on('valuechanged', buffer => {
  console.log(buffer)
})
await characteristic2.stopNotifications()
*/

//Set temperature value for uploading
var TemperatureValue = parseFloat(String.fromCharCode.apply(null, buffer), 10)

//Set battery value need it for autoML (we add a default value of 3.0)
var BatteryValue = 3.0

//Set mote value (this arduino is 1.0)
var MoteValue = 1.0;

//Set date, we subtract 3600000 because we need to be in GMT time
var DateValue = Date.now() - 3600000
console.log('Date is')
console.log(DateValue)

//Get the predicted value
    //Set payload (the data that server is sending for getting the predicted value)
    const payload = {
      row: {
          columnSpecIds: [
              "1534619165212540928",
              "6146305183639928832",
              "3840462174426234880"
            ],
          values: [
              {numberValue: BatteryValue},
              {numberValue: MoteValue},
              {stringValue: DateValue.toString()}
            ]
        }
      }
    //Create the request variable that server is sending
    const request = {
        name: modelFullId,
        payload: payload
      }
      
//Make the prediction
    client.predict(request)
      .then(responses => {
          var response = responses[0]
          //DEBUGGING PURPOSES
          //console.log('Temperature predict is:')
          //console.log(response.payload[0].tables.value.numberValue)
          //console.log('End of temperature predict')
          // Create a predict record to be stored in the database
          var visit = {
            Timestamp: DateValue,
            Mote: MoteValue,
            Temperature: TemperatureValue,
            TemperaturePredict: response.payload[0].tables.value.numberValue,
            Battery: BatteryValue
            };
            try {
              insertVisit(visit);
         
            } catch (error) {
              console.log(error);
            }
        })


//Do not disconnect for now
//await device.disconnect()
console.log('End of reading the file')
console.log('Run new loop')
timeoutId.refresh()
await new Promise(resolve => setTimeout(resolve, 10000))
}//while(1) bracket
destroy()

	}
	
  main()
	.then(console.log)
	.catch(console.error)

