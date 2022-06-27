const path = require('path');
const fs = require('fs');
const zlib = require("zlib");

const directoryPath = path.join("C:/2021/BaiGe/GeoProcess/TerrainModified");
//const directoryPath = path.join("C:/2021/BaiGe/GeoProcess/TerrainRaw");

filenames = []
fileBins = {}
fileBins_deflated = {}
fs.readdir(directoryPath, function (err, files) {
    //handling error
    if (err) {
        return console.log('Unable to scan directory: ' + err);
    }
    //listing all files using forEach
    files.forEach(function (file) {
        // Do whatever you want to do with the file
        binary = fs.readFileSync(path.join(directoryPath, file));
        d_binary = zlib.deflateSync(binary); // The size of deflated file matched!
        fileBins[file] = binary;
        fileBins_deflated[file] = d_binary;
        filenames.push(file);
        console.log(`${file} has been loaded.`);
    });
});

/* eslint-disable @typescript-eslint/no-var-requires */
// https://stackoverflow.com/a/63602976/470749
const express = require('express');

const app = express();
const https = require('https');
const http = require('http');
// const { response } = require('express');

//const targetUrl = process.env.TARGET_URL || 'http://t5.tianditu.gov.cn/mapservice/swdx?tk=530acfb193787c3172d42c8cbcac185a&x=7&y=7&l=6'; // Run localtunnel like `lt -s rscraper -p 8080 --print-requests`; then visit https://yourname.loca.lt/todos/1 .

const proxyServerPort = process.env.PROXY_SERVER_PORT || 3000;


const targetUrls = [
    "http://t0.tianditu.gov.cn/",
    "http://t1.tianditu.gov.cn/",
    "http://t2.tianditu.gov.cn/",
    "http://t3.tianditu.gov.cn/",
    "http://t4.tianditu.gov.cn/",
    "http://t5.tianditu.gov.cn/",
    "http://t6.tianditu.gov.cn/",
    "http://t7.tianditu.gov.cn/",
]

var rotation = 0;

var dumb_mutex = {};
var usable_flag = {};
for (i = 0; i < targetUrls.length; i++) {
    dumb_mutex[targetUrls[i]] = false;
    usable_flag[targetUrls[i]] = true;
}

function check(){
    console.log("\n当前服务器连通性：")
    for(i in usable_flag){
        console.log(`${i} : ${usable_flag[i]}`)
    }
    console.log("#########")
};

setInterval(check, 5000);

function sleep(ms){
    return new Promise(resolve => setTimeout(resolve,ms))
};


// localhost:3000/mapservice/swdx?tk=530acfb193787c3172d42c8cbcac185a&x=98&y=20&l=7
// eslint-disable-next-line max-lines-per-function
app.get('/*', async function (clientRequest, clientResponse) {
    while(true){
        var targetUrl = targetUrls[rotation];
        if(usable_flag[targetUrl])
            break;
        rotation = (rotation + 1) % targetUrls.length;
    }

    while(dumb_mutex[targetUrl]){
        await sleep(500);
    }
    dumb_mutex[targetUrl] = true;


    //console.log(`Rotation : ${rotation}`);

    const parsedHost = targetUrl.split('/').splice(2).splice(0, 1).join('/');
    let parsedPort;
    let parsedSSL;
    if (targetUrl.startsWith('https://')) {
        parsedPort = 443;
        parsedSSL = https;
    } else if (targetUrl.startsWith('http://')) {
        parsedPort = 80;
        parsedSSL = http;
    }
    const options = {
        hostname: parsedHost,
        port: parsedPort,
        path: clientRequest.url,
        method: clientRequest.method,
        headers: {
            'User-Agent': clientRequest.headers['user-agent'],
        },
    };

    const serverRequest = parsedSSL.request(options, function (serverResponse) {
        var parts = options.path.split("&");
        var x = Number(parts[1].split("=")[1]);
        var y = Number(parts[2].split("=")[1]);
        var l = Number(parts[3].split("=")[1]);

        var tag = `${l}_${x}_${y}`;

        if (serverResponse.statusCode === 418) {
            usable_flag[targetUrl] = false;
            console.log(`This address has been blocked by ${options.hostname}.`);
        }
        if (serverResponse.statusCode !== 200) {
            console.log(`Strange packaged returned by ${options.hostname}. Error code: ${serverResponse.statusCode}`, );
        }
        let body = '';
        if (String(serverResponse.headers['content-type']).indexOf('text/html') !== -1) {
            serverResponse.on('data', function (chunk) {
                body += chunk;
            });

            serverResponse.on('end', function () {
                // Make changes to HTML files when they're done being read.
                // body = body.replace(`example`, `Cat!`);

                clientResponse.writeHead(serverResponse.statusCode, serverResponse.headers);
                clientResponse.end(body);
                dumb_mutex[targetUrl] = false;
            });
        } else if (fileBins_deflated[tag] !== undefined) {
            serverResponse.on('data', function (chunk) {
                body += chunk;
            });

            serverResponse.on('end', function () {
                // Make changes to HTML files when they're done being read.
                // body = body.replace(`example`, `Cat!`);
                console.log(`Catch request for ${tag}`);
                body = fileBins_deflated[tag];

                clientResponse.writeHead(serverResponse.statusCode, serverResponse.headers);
                clientResponse.end(body);
                dumb_mutex[targetUrl] = false;
            });
        } else {
            serverResponse.pipe(clientResponse, {
                end: true,
            });

            clientResponse.contentType(serverResponse.headers['content-type']);
            dumb_mutex[targetUrl] = false;
        }
    });

    serverRequest.end();
});

app.listen(proxyServerPort);
console.log(`Proxy server listening on port ${proxyServerPort}`);