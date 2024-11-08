// Dependencies
var createError = require('http-errors');
var express = require('express');
var path = require('path');
var cookieParser = require('cookie-parser');
var logger = require('morgan');
var mqtt = require('mqtt');

// Express. Config app.
var app = express();
app.set('views', path.join(__dirname, 'views'));
app.set('view engine', 'ejs');

// Morgan. Config logging.
app.use(logger('dev'));
app.use(express.json());
app.use(express.urlencoded({ extended: false }));
app.use(cookieParser());
app.use("/public", express.static(path.join(__dirname, 'public')));

// Express. Define routes.
var router = require('./routes/index');
app.use('/', router);


// Http-Erros. Catch errors.
app.use(function(req, res, next) {
  next(createError(404));
});
app.use(function(err, req, res, next) {
  res.locals.message = err.message;
  res.locals.error = req.app.get('env') === 'development' ? err : {};
  res.status(err.status || 500);
  res.render('error');
});

// Sequelize. Load models.
var models = require('./models');

// Socket.io. Start websockets.
var io = require('./bin/www');

// MQTT. Connect to broker.
var client = mqtt.connect('mqtt://127.0.0.1');
client.on('connect', function() {
  client.subscribe('#');
});

// MQTT. Handle incoming StallSense sensors.
client.on('message', (topic, message) => {
  var msg = message.toString();

  // Handle a state update.
  if (msg === '0' || msg === '1') {
    var stallstate = false;
    if (msg === '1') {
      stallstate = true;
    }
    models.sensors.findAll({
      where: {
        location: topic,
      },
      raw: true
    })
    .then(function(data) {
      if (data[0].status != msg) {
        models.sensors.update(
          {status: stallstate},
          { where: { location: topic } }
        )
        .then(function() {
          models.sensors.findAll({
            where: {
              location: topic,
            },
            raw: true
          })
          .then(function(newdata) {
            newdata[0].updatedAt.setHours(newdata[0].updatedAt.getHours() - 7);
            newdata[0].updatedAt = newdata[0].updatedAt.toLocaleString();
            io.sockets.emit('statusupdate', newdata[0]);
          })
        })
      }
    });
  }

  // Handle a potential broadcast.
  else if (topic.split('/').pop() === 'id') {
    if (msg.split('.')[1] === 'StallSense') {
      var loc = topic.slice(0, -3);
      models.sensors.count({ where: { location: loc } })
        .then(function(count) {
          console.log(count);
          if (count === 0) {
            models.sensors.create({
              location: loc,
              type: msg.split('.').pop()
            }).then(function () {} )
          }
        })
    }
  }

  // Handle a potential IP update.
  else if (topic.split('/').pop() === 'ip') {
    var loc = topic.slice(0, -3);
    models.sensors.update(
      {ip: msg},
      { where: { location: loc } }
    );
  }
});

module.exports = app;
