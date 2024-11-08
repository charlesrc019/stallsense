'use strict';
module.exports = (sequelize, DataTypes) => {
  const sensors = sequelize.define('sensors', {
    location: DataTypes.STRING,
    status: DataTypes.BOOLEAN,
    type: DataTypes.STRING,
    ip: DataTypes.STRING,
    updatedAt: DataTypes.DATE
  }, {});
  sensors.associate = function(models) {
    // associations can be defined here
  };
  return sensors;
};
