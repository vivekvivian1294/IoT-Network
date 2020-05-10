// Copyright 2017 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// [START gae_flex_datastore_app]
'use strict';

const express = require('express');
const crypto = require('crypto');
const { exec } = require("child_process");
const app = express();
app.enable('trust proxy');

// By default, the client will authenticate using the service account file
// specified by the GOOGLE_APPLICATION_CREDENTIALS environment variable and use
// the project specified by the GOOGLE_CLOUD_PROJECT environment variable. See
// https://github.com/GoogleCloudPlatform/google-cloud-node/blob/master/docs/authentication.md
// These environment variables are set automatically on Google App Engine
const {Datastore} = require('@google-cloud/datastore');

// Instantiate a datastore client
const datastore = new Datastore();
//Get the automl libraries
const automl = require('@google-cloud/automl')

//Set client options to europe
const clientOptions = {apiEndpoint: 'eu-automl.googleapis.com'}

const client = new automl.v1beta1.PredictionServiceClient(clientOptions)

//Set project id
const projectID = 'christos901129test'
//Set computed Region
const computeRegion = 'eu'
//Set modelID
const modelId = 'TBL4353116067946561536'

//Get the full path of the model.
const modelFullId = client.modelPath(projectID, computeRegion, modelId)


/**
 * Insert a visit record into the database.
 *
 * @param {object} visit The visit record to insert.
 */
 
 const insertVisit = visit => {
  return datastore.save({
    key: datastore.key('visit'),
    data: visit,
  });
};

const getTimer = timer => {
  const query = datastore
        .createQuery('timer')
        .order('Timestamp', {descending: true})
        .limit(1);
  } 

const mote = exec("./serialdump-linux -T -b115200 /dev/ttyUSB0")

console.log("", mote.pid, "started")

mote.stdout.on('data', async (data) =>{
  //while mote received data echo a message
  
  //const {timer} = await getTimer()
  //console.log(timer)
  //THIS DOESNT WORK 100%
  //const sendMote = exec('echo "300" > /dev/ttyUSB0')
  //allowed input for regular expression
  let allowedInput = '[0-9][0-9][0-9][0-9][-][0-9][0-9][-][0-9][0-9][T][0-9][0-9]:[0-9][0-9]:[0-9][0-9][.][0][0][0][Z][|]'+
  '[T][e][m][p][=][ -][0-9]+[.][0-9]+[,]'+
  '[B][a][t][t][e][r][y][=][0-9]+[.][0-9]+[,]'+
  '[M][o][t][e][=][0-9]+[.][0-9]'
  const allowedInputRegex = RegExp(allowedInput)
  process.stdout.write(data)
  if(allowedInputRegex.test(data))
  {
    //Get date variable
    let DateStringInput = '[0-9][0-9][0-9][0-9][-][0-9][0-9][-][0-9][0-9][T][0-9][0-9]:[0-9][0-9]:[0-9][0-9][.][0][0][0][Z]'
    const DateStringRegex = RegExp(DateStringInput)
    let DateStringData = DateStringRegex.exec(data)
    let DateValue = Date.parse(DateStringData)
    //remove 3600000 for 1 hour in ms (we want gmt time not swedish time)
    DateValue = DateValue - 3600000
    
    
    //Get temperature variable
    let TempStringInput = '[T][e][m][p][=][ -][0-9]+[.][0-9]+'
    const TempStringRegex = RegExp(TempStringInput)
    let TempStringData = TempStringRegex.exec(data)
    let TempStringDataInput = '[-]?[0-9]+[.][0-9]+'
    const TempStringDataRegex = RegExp(TempStringDataInput)
    let TemperatureValue = TempStringDataRegex.exec(TempStringData)
    TemperatureValue = parseFloat(TemperatureValue, 10)

    
    //Get battery variable
    let BatteryStringInput = '[B][a][t][t][e][r][y][=][0-9]+[.][0-9]+'
    const BatteryStringRegex = RegExp(BatteryStringInput)
    let BatteryStringData = BatteryStringRegex.exec(data)
    let BatteryStringDataInput = '[0-9]+[.][0-9]+'
    const BatteryStringDataRegex = RegExp(BatteryStringDataInput)
    let BatteryValue = TempStringDataRegex.exec(BatteryStringData)
    BatteryValue = parseFloat(BatteryValue, 10)
    
    //Get Mote Variable
    let MoteStringInput = '[M][o][t][e][=][0-9]+[.][0-9]'
    const MoteStringRegex = RegExp(MoteStringInput)
    let MoteStringData = MoteStringRegex.exec(data)
    let MoteStringDataInput = '[0-9]+[.][0-9]'
    const MoteStringDataRegex = RegExp(MoteStringDataInput)
    let MoteValue = MoteStringDataRegex.exec(MoteStringData)
    MoteValue = parseFloat(MoteValue, 10)
    
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
          const response = responses[0]
          //DEBUGGING PURPOSES
          //console.log('Temperature predict is:')
          //console.log(response.payload[0].tables.value.numberValue)
          //console.log('End of temperature predict')
          // Create a predict record to be stored in the database
          const visit = {
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
      
  }
  })
  
  

/*This function is used to read the serial data
mote.stdout.on('data', (data) =>{
  
  process.stdout.write(data)
  })
*/
mote.stderr.on('data', (data) =>{
  console.log("STDERR: "+data)
  })

mote.on('close', (code, signal) => {
  console.log(`child process terminated due to receipt of signal ${signal}`)
  })
/*
setInterval(() => {
  mote.stdout.on('data', (data) => {
    console.log(data)
    })
  }, 2000)

*/
