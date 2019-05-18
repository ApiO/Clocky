fs     = require('fs');
path   = require('path');
mkdirp = require('../common/node_modules/mkdirp');
PNG    = require('../common/node_modules/pngjs').PNG;
util   = require('util');

function resize_bottom() {
  $('#config-bottom').width($('.content').width() - 2);
}

function init() {
	$("#config").hide();
	// prevent default behavior from changing page on dropped file
	window.ondragover = function(e) { e.preventDefault(); return false };
	window.ondrop     = function(e) { e.preventDefault(); return false };

  $(window).resize(resize_bottom);

	$("#holder")
	.on("dragover",  function ()  { this.className = 'hover'; return false; })
	.on("dragleave", function ()  { this.className = ''; return false; })
	.on("drop",      function (e) { this.className = '';
	  e.preventDefault();
	  for (var i = 0; i < e.originalEvent.dataTransfer.files.length; ++i)
	    load_model(e.originalEvent.dataTransfer.files[i].path);
	  return false;
	});
}

function load_model(file) {
  var m             = {};
  m.name            = path.basename(file, ".json");
  m.input           = file;
  m.actor_templates = [];
  m.shape_templates = [];
  m.materials       = [];
  m.override        = true;

  fs.readFile(source_path + '/settings.pgconf', 'utf8', function (err,data) {
    var settings = JSON.parse(data);
    fs.readFile(source_path + '/' + settings.boot.physics + '.pgphys', 'utf8', function (err,data) {
      var physics = JSON.parse(data);
      for (var i in physics.actors)    m.actor_templates.push(i);
      for (var i in physics.shapes)    m.shape_templates.push(i);
      for (var i in physics.materials) m.materials.push(i);

      fs.readFile(file, 'utf8', function (err,data) {
        if (err) return console.log(err);
        var src = JSON.parse(data);
        m.bodies = src.rigidBodies;
        for (var i in m.bodies) {
          m.bodies[i].actor_template = 0;
          m.bodies[i].shape_template = 0;
          m.bodies[i].material       = 0;
          m.bodies[i].selected       = true;
        }
        model_loaded(m);
      });
    });
  });
}

function render_dropdown(data)
{
  var result = '<div class="input-control select"><select>';
  for (var i in data)
    result += util.format('<option>%s</option>', data[i]);
  result += '</select></div>';
  return result;
}

function render_checkbox()
{
  return '<div class="input-control checkbox"><span class="check"></span></div>';
}

function populate_select(selector, data)
{
  var select = $(selector);
  $.each(data, function(index, value) {
      select.append($("<option />").val(index).text(value));
  });
}

function model_loaded(model) {
  for (var e in model.bodies) {
    var r = '<tr>';
    r += '<td><div class="input-control checkbox"><label><input type="checkbox" class="select_row"><span class="check"></span></label></div></td>';
    r += '<td>' + model.bodies[e].name + '</td>'
    r += '<td><div class="input-control select"><select class="actor_tpl"></select></div></td>'
    r += '<td><div class="input-control select"><select class="shape_tpl"></select></div></td>'
    r += '<td><div class="input-control select"><select class="material"></select></div></td>';
    $(r).appendTo('#units > tbody');
  }

  $('.select_row, .select_all').prop('checked', true);
  $('.select_all').click(function() {
    $('.select_row, .select_all').prop('checked', this.checked);
  });

  $('#actor_tpl_all').change(function() { $('.actor_tpl').val($(this).val()); });
  $('#shape_tpl_all').change(function() { $('.shape_tpl').val($(this).val()); });
  $('#material_all').change(function()  { $('.material').val($(this).val()); });

  populate_select('.actor_tpl', model.actor_templates);
  populate_select('.shape_tpl', model.shape_templates);
  populate_select('.material',  model.materials);

  $('#submit').click(function() {
    update_model(model);
    import_model(model);
  });

  $("#holder").hide();
  $("#name").val(model.name);
  resize_bottom();
  $("#config").show();
}

function update_model(model) {
  $('#units>tbody>tr').each(function(index, value) {
    var row      = $(value);
    var m        = model.bodies[index];
    m.selected   = row.find(".select_row").is(':checked');
    m.actor_tpl  = model.actor_templates[row.find(".actor_tpl").val()];
    m.shape_tpl  = model.shape_templates[row.find(".shape_tpl").val()];
    m.material   = model.materials[row.find(".material").val()];
  });
  model.rel_output = $('#name').val(); 
  model.abs_output = source_path + '/' + model.rel_output;
}

// -----------------
// import processing
// -----------------

function create_atlas(name) {
  var atlas = fs.createWriteStream(name);
  atlas.write('{\n\t"type": "ATLAS",\n\t"regions": {');
  return atlas;
}

function close_atlas(atlas) {
  atlas.end('\n\t}\n}');
}

function create_resources(model, body, atlas, last) {
  var p = path.resolve(path.dirname(model.input), body.imagePath);
  var f = fs.createReadStream(p)
    .pipe(new PNG({filterType: 4}))
    .on('parsed', function() {
      var actor = { "template": body.actor_tpl, "shapes": {} };
      var shape_translation = [ 
        -1 * this.width/2  + body.origin.x, 
        -1 * this.height/2 + body.origin.y];
      // create shapes
      for(var i in body.polygons) {
        var poly = body.polygons[i];
        var shape_name = util.format("shape_%d", i);
        var shape_file = util.format("%s_%s", body.name, i);

        actor.shapes[shape_name] = { 
          "template" : body.shape_tpl, 
          "material" : body.material,
          "shape"    : model.rel_output + "/" + shape_file,
          "translation" : shape_translation
        };

        var shape = {
          "type"  : "POLYGON",
          "value" : []
        };

        for (var j in poly) {
          shape.value.push(Math.round(poly[j].x * this.width));
          shape.value.push(Math.round(poly[j].y * this.width));
        }
        fs.createWriteStream(model.abs_output + "/" + shape_file + ".pgshape")
          .write(JSON.stringify(shape, null, 2));
      }

      // write the actor
      fs.createWriteStream(util.format("%s/%s.pgactor", model.abs_output, body.name))
        .write(JSON.stringify(actor, null, 2));

      // copy the image into the export folder
      var img_file = util.format("textures/%s.png", body.name);
      this.pack().pipe(fs.createWriteStream(model.abs_output + "/" + img_file));

      // write the atlas
      atlas.write(util.format('\n\t\t"%s": { "file": "%s/%s" }', body.name, model.rel_output, img_file));
      if (last) 
        close_atlas(atlas);
      else
        atlas.write(',');

      // create the sprite
      var sprite = {
        "setup_frame" : "default",
        "frames" : {
          "default" : {
            "texture" : model.rel_output + "/atlas#" + body.name
          }
        }
      };
      
      fs.createWriteStream(util.format("%s/%s.pgsprite", model.abs_output, body.name))
        .write(JSON.stringify(sprite, null, 2));

      // create the unit
      var unit = {
        "nodes" : { "root" : {} },
        "sprites" : {
          "main" : {
            "node" : "root",
            "template" : model.rel_output + "/" + body.name,
            "order" : 0
          }
        },
        "actors" : {
          "main" : {
            "node" : "root",
            "actor" : model.rel_output + "/" + body.name
          }
        }
      };

      fs.createWriteStream(util.format("%s/%s.pgunit", model.abs_output, body.name))
        .write(JSON.stringify(unit, null, 2));
    });
}

function import_model(model)  {
  alert(model.abs_output);
  mkdirp.sync(model.abs_output);
  mkdirp.sync(model.abs_output + "/textures");
  var atlas = create_atlas(model.abs_output + "/atlas.pgtex");

  for(var i in model.bodies) {
    if (!model.bodies[i].selected){
      alert('skipped');
      continue;
    }

    var body = model.bodies[i];
    create_resources(model, body, atlas, i == model.bodies.length - 1);
  }
  alert('done');
}