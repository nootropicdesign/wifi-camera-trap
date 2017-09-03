var express = require('express');
var app = express();
var path = require('path');
var fs = require('fs');
var dateFormat = require('dateformat');
var jimp = require("jimp");

var uploadDir = 'public/uploads';
app.set('view engine', 'pug');
app.use(express.static('public'));

app.get('/images', (req, res) => {
    let images = getImagesFromDir(path.join(__dirname, 'public/uploads'));
    res.render('index', { title: 'Camera Trap Image Server', images: images })
});


function getImagesFromDir(dirPath) {
    let allImages = [];
 
    let files = fs.readdirSync(dirPath);
 
    for (file of files) {
        let fileLocation = path.join(dirPath, file);
        var stat = fs.statSync(fileLocation);
        if (stat && stat.isDirectory()) {
            getImagesFromDir(fileLocation); // process sub directories
        } else if (stat && stat.isFile() && ['.jpg', '.png'].indexOf(path.extname(fileLocation)) != -1) {
            allImages.push('uploads/'+file);
        }
    }
    return allImages;
}




app.post('/upload', function(req, res){
  var d = new Date()
  var filename = dateFormat(new Date(), "mm-dd-yyyy_HH_MM_ss") + ".jpg";
  filename = path.join(__dirname, uploadDir + '/' + filename);
  var f = fs.createWriteStream(filename);
  req.on('data', function(chunk) {
    f.write(chunk);
  }).on('end', function() {
    f.end();    

    // rotate image 180 degrees because camera is mounted upside-down
    jimp.read(filename, function (err, image) {
      image.rotate(180)
           .write(filename);
    }).catch(function (err) {
      console.log("error: " + err);
    });

    console.log("saved " + filename);
    res.status(200).end("OK");
  });

});

var server = app.listen(8000, function(){
  console.log('Server listening on port 8000');
});
