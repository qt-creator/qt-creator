/* START non link areas where cursor should change to pointing hand */
$('.t_button').mouseover(function() {
    $('.t_button').css('cursor','pointer');
		/*document.getElementById(this.id).style.cursor='pointer';*/
});
/* END non link areas  */
$('#smallA').click(function() {
		$('.content .heading,.content h1, .content h2, .content h3, .content p, .content li, .content table').css('font-size','smaller');
		$('.t_button').removeClass('active')
		$(this).addClass('active')
});

$('#medA').click(function() {
		$('.content .heading').css('font','600 16px/1 Arial');
		$('.content h1').css('font','600 18px/1.2 Arial');
		$('.content h2').css('font','600 16px/1.2 Arial');
		$('.content h3').css('font','600 14px/1.2 Arial');
		$('.content p').css('font','13px/20px Verdana');
		$('.content li').css('font','400 13px/1 Verdana');
		$('.content li').css('line-height','14px');
		$('.content .toc li').css('font', 'normal 10px/1.2 Verdana');
		$('.content table').css('font','13px/1.2 Verdana');
		$('.content .heading').css('font','600 16px/1 Arial');
		$('.content .indexboxcont li').css('font','600 13px/1 Verdana');
		$('.t_button').removeClass('active')
		$(this).addClass('active')
});

$('#bigA').click(function() {
		$('.content .heading,.content h1, .content h2, .content h3, .content p, .content li, .content table').css('font-size','large');
		$('.content .heading,.content h1, .content h2, .content h3, .content p, .content li, .content table').css('line-height','25px');
		$('.t_button').removeClass('active')
		$(this).addClass('active')
});

$('.feedclose').click(function() {
	$('.bd').show();
	$('.hd').show();
	$('.footer').show();
	$('#feedbackBox').hide();
	$('#blurpage').hide();
});

$('.feedback').click(function() {
	$('.bd').hide();
	$('.hd').hide();
	$('.footer').hide();
	$('#feedbackBox').show();
	$('#blurpage').show();
});
var lookupCount = 0;
var articleCount = 0;
var exampleCount = 0;
var qturl = ""; // change from "http://doc.qt.nokia.com/4.6/" to 0 so we can have relative links

function processNokiaData(response){
	var propertyTags = response.getElementsByTagName('page');
	
 	for (var i=0; i< propertyTags.length; i++) {
		var linkStart   = "<li class=\"liveResult\"><a href='"+qturl+"";
		var linkEnd  = "</a></li>";
		
		if(propertyTags[i].getElementsByTagName('pageType')[0].firstChild.nodeValue == 'APIPage'){
			lookupCount++;

			for (var j=0; j< propertyTags[i].getElementsByTagName('pageWords').length; j++){
				full_li_element = linkStart + propertyTags[i].getElementsByTagName('pageUrl')[j].firstChild.nodeValue;
				full_li_element = full_li_element + "'>" + propertyTags[i].getElementsByTagName('pageTitle')[0].firstChild.nodeValue + linkEnd;
				$('#ul001').append(full_li_element);
			$('#ul001 .defaultLink').css('display','none');

		   		}
			}
	 
		if(propertyTags[i].getElementsByTagName('pageType')[0].firstChild.nodeValue == 'Article'){
			articleCount++;

			for (var j=0; j< propertyTags[i].getElementsByTagName('pageWords').length; j++){
			    full_li_element = linkStart + propertyTags[i].getElementsByTagName('pageUrl')[j].firstChild.nodeValue;
				full_li_element =full_li_element + "'>" + propertyTags[i].getElementsByTagName('pageTitle')[0].firstChild.nodeValue + linkEnd ;
					
				$('#ul002').append(full_li_element);
			$('#ul002 .defaultLink').css('display','none');

	   		}
		}
		if(propertyTags[i].getElementsByTagName('pageType')[0].firstChild.nodeValue == 'Example'){
			exampleCount++;


			for (var j=0; j< propertyTags[i].getElementsByTagName('pageWords').length; j++){
			    full_li_element = linkStart + propertyTags[i].getElementsByTagName('pageUrl')[j].firstChild.nodeValue;
				full_li_element =full_li_element + "'>" + propertyTags[i].getElementsByTagName('pageTitle')[0].firstChild.nodeValue + linkEnd ;
					
				$('#ul003').append(full_li_element);
			$('#ul003 .defaultLink').css('display','none');

	   		}
		} 
		if(i==propertyTags.length){$('#pageType').removeClass('loading');}

	}	
	if(lookupCount > 0){$('#ul001 .menuAlert').remove();$('#ul001').prepend('<li class=\"menuAlert liveResult hit\">Found ' + lookupCount + ' hits</li>');$('#ul001 li').css('display','block');$('.sidebar .search form input').removeClass('loading');}
    if(articleCount > 0){$('#ul002 .menuAlert').remove();$('#ul002').prepend('<li class=\"menuAlert liveResult hit\">Found ' + articleCount + ' hits</li>');$('#ul002 li').css('display','block');}
	if(exampleCount > 0){$('#ul003 .menuAlert').remove();$('#ul003').prepend('<li class=\"menuAlert liveResult hit\">Found ' + articleCount + ' hits</li>');$('#ul003 li').css('display','block');}
	 
	if(lookupCount == 0){$('#ul001 .menuAlert').remove();$('#ul001').prepend('<li class=\"menuAlert liveResult noMatch\">Found no result</li>');$('#ul001 li').css('display','block');$('.sidebar .search form input').removeClass('loading');}
    if(articleCount == 0){$('#ul002 .menuAlert').remove();$('#ul002').prepend('<li class=\"menuAlert liveResult noMatch\">Found no result</li>');$('#ul002 li').css('display','block');}
	if(exampleCount == 0){$('#ul003 .menuAlert').remove();$('#ul003').prepend('<li class=\"menuAlert liveResult noMatch\">Found no result</li>');$('#ul003 li').css('display','block');}
	// reset count variables;
	 lookupCount=0;
	 articleCount = 0;
     exampleCount = 0;
	
}
//build regular expression object to find empty string or any number of blank
var blankRE=/^\s*$/;
function CheckEmptyAndLoadList()
{
	var pageUrl = window.location.href;
	var pageVal = $('title').html();
	$('#feedUrl').remove();
	$('#pageVal').remove();
	$('.menuAlert').remove();
	$('#feedform').append('<input id="feedUrl" name="feedUrl" value="'+pageUrl+'" style="display:none;">');
	$('#feedform').append('<input id="pageVal" name="pageVal" value="'+pageVal+'" style="display:none;">');
	$('.liveResult').remove();
    $('.defaultLink').css('display','block');
	var value = document.getElementById('pageType').value; 
	if((blankRE.test(value)) || (value.length < 3))
	{
	//empty inputbox
		// load default li elements into the ul if empty
	//	loadAllList(); // replaced
	 $('.defaultLink').css('display','block');
	// $('.liveResult').css('display','none');
	}else{
	 $('.defaultLink').css('display','none');
	}
}
/*
$(window).resize(function(){
if($(window).width()<400)
	$('body').addClass('offline');
else
	$('body').removeClass('offline');
	});
	*/
// Loads on doc ready
	$(document).ready(function () {
	//alert(pageUrl);
	//$('#pageUrl').attr('foo',pageUrl);
	var pageTitle = $('title').html();
          var currentString = $('#pageType').val() ;
		  if(currentString.length < 1){
			$('.defaultLink').css('display','block');
      	   		CheckEmptyAndLoadList();			
		  }

        $('#pageType').keyup(function () {
          var searchString = $('#pageType').val() ;
          if ((searchString == null) || (searchString.length < 3)) {
				$('#pageType').removeClass('loading');
				 $('.liveResult').remove(); 
				 $('.searching').remove(); 
      	   		CheckEmptyAndLoadList();
				$('.report').remove();
				// debug$('.content').prepend('<li>too short or blank</li>'); // debug
				return;
		   }
            if (this.timer) clearTimeout(this.timer);
            this.timer = setTimeout(function () {
				$('#pageType').addClass('loading');
				$('.searching').remove(); 
				$('.list ul').prepend('<li class="menuAlert searching">Searching...</li>');
               $.ajax({
                contentType: "application/x-www-form-urlencoded",
                url: 'http://' + location.host + '/nokiasearch/GetDataServlet',
                data: 'searchString='+searchString,
                dataType:'xml',
				type: 'post',	 
                success: function (response, textStatus) {

				$('.liveResult').remove(); 
				$('.searching').remove(); 
				$('#pageType').removeClass('loading');
				$('.list ul').prepend('<li class="menuAlert searching">Searching...</li>');
                processNokiaData(response);

 }     
              });
            }, 500);
        });
      }); 
