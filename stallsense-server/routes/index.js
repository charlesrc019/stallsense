var express = require('express');
var Sequelize = require('sequelize');
var mqtt = require('mqtt');
var router = express.Router();
var models = require('../models');

/* GET main page */
router.get('/', function(req, res, next) {
  var dayago = new Date();
  dayago.setHours(dayago.getHours() - 24);
  models.sensors.findAll({
    where: {
      status: {[Sequelize.Op.not]: null},
    },
    order: [
      ['location', 'ASC']
    ],
    raw: true
  })
    .then(function(data) {
      data.forEach(function(entry) {
        var loc_tree = entry.location.split('/');
        loc_tree[loc_tree.length-1] = "<strong>" + loc_tree[loc_tree.length-1] + "</strong>";
        entry.location = loc_tree.join(" > ");
        var status_new = "EMPTY";
        if (entry.status === 1) {
          status_new = "OCCUPIED";
        }
        entry.status = status_new;
        entry.updatedAt.setHours(entry.updatedAt.getHours() - 7);
        entry.updatedAt = entry.updatedAt.toLocaleString();
      })
      return data;
    }).then(function(data) {
      
      res.render('index', {page: 'Status', sensors: data});
    });
});

/* GET reset page */
router.get('/reset/:id', function(req, res, next) {
  models.sensors.findAll({
    where: {
      id: req.params.id
    },
    raw: true
  })
  .then(function(data) {
    var client = mqtt.connect('mqtt://127.0.0.1');
    client.on('connect', function() {
      client.publish(data[0].location + "/rst", "1");
    })
  })
  .then(function() {
    models.sensors.destroy({
      where: {
          id: req.params.id
      }
    })
  })
  .then(function() {
    res.redirect('/');
  })
});

module.exports = router;
