var narrowInit = function() {
  /* TODO:
  Could probably be more efficient, not hardcoding each element to be created
  */
  // 1: Create search form
  var narrowSearch = $('<div id="narrowsearch"></div>');
  var searchform = $("#qtdocsearch");
  narrowSearch.append(searchform);
  $("#qtdocheader .content .qtref").after(narrowSearch);

  // 2: Create dropdowns
  var narrowmenu = $('<ul id="narrowmenu" class="sf-menu"></ul>');

  // Lookup
  var lookuptext = $("#lookup h2").attr("title");
  $("#lookup ul").removeAttr("id");
  $("#lookup ul li").removeAttr("class");
  $("#lookup ul li").removeAttr("style");
  var lookupul = $("#lookup ul");
  var lookuplist = $('<li></li>');
  var lookuplink = $('<a href="#"></a>');
  lookuplink.append(lookuptext);
  lookuplist.append(lookuplink);
  lookuplist.append(lookupul);
  narrowmenu.append(lookuplist);

  // Topics
  var topicstext = $("#topics h2").attr("title");
  $("#topics ul").removeAttr("id");
  $("#topics ul li").removeAttr("class");
  $("#topics ul li").removeAttr("style");
  var topicsul = $("#topics ul");
  var topicslist = $('<li></li>');
  var topicslink = $('<a href="#"></a>');
  topicslink.append(topicstext);
  topicslist.append(topicslink);
  topicslist.append(topicsul);
  narrowmenu.append(topicslist);

  // Examples
  var examplestext = $("#examples h2").attr("title");
  $("#examples ul").removeAttr("id");
  $("#examples ul li").removeAttr("class");
  $("#examples ul li").removeAttr("style");
  var examplesul = $("#examples ul");
  var exampleslist = $('<li></li>');
  var exampleslink = $('<a href="#"></a>');
  exampleslink.append(examplestext);
  exampleslist.append(exampleslink);
  exampleslist.append(examplesul);
  narrowmenu.append(exampleslist);

  $("#shortCut").after(narrowmenu);
  $('ul#narrowmenu').superfish({
    delay: 100,
    autoArrows: false,
    disableHI: true
  });
}

$(document).ready(function(){
/*   if ($('body').hasClass('narrow')) {
    narrowInit();
  }
 */
 if($(window).width()<600) {
    $('body').addClass('narrow');

    if ($("#narrowsearch").length == 0) {
      narrowInit();
    }
  }
  else {
    $('body').removeClass('narrow');
  }
});

$(window).bind('resize', function () {
  if($(window).width()<600) {
    $('body').addClass('narrow');

    if ($("#narrowsearch").length == 0) {
      narrowInit();
    }
  }
  else {
    $('body').removeClass('narrow');
  }
});