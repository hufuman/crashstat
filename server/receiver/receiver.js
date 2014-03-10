var http=require('http');
var url=require("url");
var path=require("path");
var fs=require("fs");
var formidable=require('formidable');
var sys = require("sys");


  http.createServer(function (request, response)
  {
    var endData = function()
    {
      console.log("\n\n");
      response.end();
    }

    var uri=url.parse(request.url).pathname;

    console.log(uri);
    if(uri != "/uploadDmp")
    {
        response.writeHead(404, {"Content-type": "text/plain"});
        response.end("resource associated with this uri");
        return;
    }

    var form = new formidable.IncomingForm();
    form.parse(request, function(err, fields, files)
    {
        if(err)
        {
            console.log("Error: " + err);
            response.writeHead(500, {"Content-type": "text/plain"});
            response.end(err);
            response.end();
        }
        else
        {
            response.writeHead(200, {"Content-type": "text/plain"});
            response.write(sys.inspect({fields: fields, files: files}));
            response.end();
        }
    });

  }).listen(8033);
