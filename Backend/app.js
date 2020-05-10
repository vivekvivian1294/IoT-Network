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
const cors = require('cors');
const bodyParser = require('body-parser');

//setEndpoint('christos901129test')
//predict_function()
/**
 * Demonstrates using the AutoML client to request prediction from
 * automl tables using csv.
 * TODO(developer): Uncomment the following lines before running the sample.
 */

// const projectId = '[PROJECT_ID]' e.g., "my-gcloud-project";
// const computeRegion = '[REGION_NAME]' e.g., "us-central1";
// const modelId = '[MODEL_ID]' e.g., "TBL4704590352927948800";

        

// import dependencies
const jwt = require("express-jwt"); 
const jwksRsa = require("jwks-rsa"); 

// Set up Auth0 configuration 
const authConfig = {
  domain: "dev-5ru9eb8m.eu.auth0.com",
  audience: "https://wireless-project-api.se"
};

// Create middleware to validate the JWT using express-jwt
const checkJwt = jwt({
  // Provide a signing key based on the key identifier in the header and the signing keys provided by your Auth0 JWKS endpoint.
  secret: jwksRsa.expressJwtSecret({
    cache: true,
    rateLimit: true,
    jwksRequestsPerMinute: 5,
    jwksUri: `https://${authConfig.domain}/.well-known/jwks.json`
  }),

  // Validate the audience (Identifier) and the issuer (Domain).
  audience: authConfig.audience,
  issuer: `https://${authConfig.domain}/`,
  algorithm: ["RS256"]
});


const app = express();
app.enable('trust proxy');
app.use(cors());
app.use(bodyParser.json());
app.use(express.urlencoded( { extended: true}));

// By default, the client will authenticate using the service account file
// specified by the GOOGLE_APPLICATION_CREDENTIALS environment variable and use
// the project specified by the GOOGLE_CLOUD_PROJECT environment variable. See
// https://github.com/GoogleCloudPlatform/google-cloud-node/blob/master/docs/authentication.md
// These environment variables are set automatically on Google App Engine
const {Datastore} = require('@google-cloud/datastore');

// Instantiate a datastore client
const datastore = new Datastore();

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

/**
 * Get predicted temperatures last hour
 */
const getPredict = () => {
  const query = datastore
    .createQuery('predict')
    .order('Timestamp', {descending: true})
    .filter('Timestamp', '>=', new Date().getTime() - (1000 * 60 * 60 * 24))
    //.limit(1000);

  return datastore.runQuery(query);
};

 
/**
 * Get all visits last hour
 */
const getVisits = () => {
  const query = datastore
    .createQuery('visit')
    .order('Timestamp', {descending: true})
    .filter('Timestamp', '>=', new Date().getTime() - (1000 * 60 * 60 * 24))
    //.limit(1000);

  return datastore.runQuery(query);
};



/**
 * Get visits by id last hour
 */
const getVisitsbyId = () => {
  const query = datastore
    .createQuery('visit')
    .order('Timestamp', {descending: true})
    .filter('Timestamp', '>=', new Date().getTime() - (1000 * 60 * 60 * 24) - (1000 * 60 * 60 * 1) )
    .filter('Mote', '=', 140)
    //.limit(1000);

  return datastore.runQuery(query);
};

/**
 * Get all visits from the beggining of time
 */
const getAllVisits = () => {
  const query = datastore
    .createQuery('visit')
    .order('Timestamp', {descending: true})
    //.limit(1000);

  return datastore.runQuery(query);
};

const insertTimer = timer => {
  return datastore.save({
    key: datastore.key('timer'),
    data: timer,
  });
};

const getTimer = () => {
  const query = datastore
    .createQuery('timer')
    .order('Timestamp', {descending: true})
    .limit(1);

  return datastore.runQuery(query);
}

app.get('/api/alldata', async(req, res, next) => {
  try {
    const [entities] = await getAllVisits();
    const visits = entities.map(
      entity => `Timestamp: ${new Date(entity.Timestamp)}, Mote: ${entity.Mote}, Temperature: ${entity.Temperature}, Battery: ${entity.Battery}`
    );
    return res
    .status(200)
    .json(entities)
    .end();
    // Convert epoch timestamp to date command:
    // new Date(entity.Timestamp)}
  } catch (error) {
    next(error);
  }
})
//checkJwt, 
app.get('/api/motedata', async (req, res, next) => {
  // Create a visit record to be stored in the database
  try {
    const [entities] = await getVisits();
    const visits = entities.map(
      entity => `Timestamp: ${new Date(entity.Timestamp)}, Mote: ${entity.Mote}, Temperature: ${entity.Temperature}, Battery: ${entity.Battery}`
    );
    return res
    .status(200)
    .json(entities)
    .end();
    // Convert epoch timestamp to date command:
    // new Date(entity.Timestamp)}
  } catch (error) {
    next(error);
  }
});
//checkJwt,
app.get('/api/motedata/:id', async (req, res, next) => {
  // Create a visit record to be stored in the database
  try {
    //id = req.params.id
    const getVisitsbyId = () => {
      const query = datastore
        .createQuery('visit')
        .order('Timestamp', {descending: true})
        .filter('Mote', '=', parseInt(req.params.id))
        .filter('Timestamp', '>=', new Date().getTime() - (1000 * 60 * 60 * 8) - (1000 * 60 * 60) )
        
        //.limit(1000);
    
      return datastore.runQuery(query);
    };
    
    const [entities] = await getVisitsbyId();
    const visits = entities.map(
      entity => `Timestamp: ${new Date(entity.Timestamp)}, Mote: ${entity.Mote}, Temperature: ${entity.Temperature}, Battery: ${entity.Battery}`
    );
    return res
    .status(200)
    .json(entities)
    .end();
    // Convert epoch timestamp to date command:
    // new Date(entity.Timestamp)}
  } catch (error) {
    next(error);
  }
});
///api/predictdata/:id
app.get('/api/predictdata/:id',  async (req, res, next) => {
  // Create a visit record to be stored in the database
  try {
    //id = req.params.id
    const getVisitsbyId = () => {
      const query = datastore
        .createQuery('predict')
        .order('Timestamp', {descending: true})
        .filter('Mote', '=', parseInt(req.params.id))
        .filter('Timestamp', '>=', new Date().getTime() - (1000 * 60 * 60 * 8) - (1000 * 60 * 60) )
        
        //.limit(1000);
    
      return datastore.runQuery(query);
    };
    
    const [entities] = await getVisitsbyId();
    const visits = entities.map(
      entity => `Mote: ${new Date(entity.Mote)}, Timestamp: ${entity.Timestamp}, Temperature: ${entity.Temperature}`
    );
    return res
    .status(200)
    .json(entities)
    .end();
    // Convert epoch timestamp to date command:
    // new Date(entity.Timestamp)}
  } catch (error) {
    next(error);
  }
});

const PORT = process.env.PORT || 3000;
app.listen(process.env.PORT || 3000, () => {
  console.log(`App listening on port ${PORT}`);
  console.log('Press Ctrl+C to quit.');
});
// [END gae_flex_datastore_app]

module.exports = app;
