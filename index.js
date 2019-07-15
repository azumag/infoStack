const tts = require('@google-cloud/text-to-speech');
const fs = require('fs');
const util = require('util');

const functions = require('firebase-functions');
const express = require("express");

const cors = require('cors')
const app = express()

const URLSafeBase64 = require('urlsafe-base64');


app.use(cors())

app.get('/output', async (areq, ares) => {
  // Import other required libraries
  // Creates a client
  console.log(areq.query)
  var text = "";
  if (areq.query.text) {
    text = URLSafeBase64.decode(areq.query.text).toString().replace(" ", ",");
    // text = (new Buffer(areq.query.text, 'base64')).toString();
  }

  console.log(text)

  const client = new tts.TextToSpeechClient();

  // Construct the request
  const request = {
    input: { text: text },
    // Select the language and SSML Voice Gender (optional)
    voice: { languageCode: 'ja-JP', ssmlGender: 'NEUTRAL' },
    // Select the type of audio encoding
    audioConfig: { audioEncoding: 'MP3' },
  };

  // Performs the Text-to-Speech request
  const [response] = await client.synthesizeSpeech(request);
  // Write the binary audio content to a local file
  const writeFile = util.promisify(fs.writeFile);
  await writeFile('/tmp/output.mp3', response.audioContent, 'binary');
  console.log('Audio content written to file: output.mp3');

  ares.download('/tmp/output.mp3')
})

const textToSpeech = functions.https.onRequest(app);
module.exports = { textToSpeech };
