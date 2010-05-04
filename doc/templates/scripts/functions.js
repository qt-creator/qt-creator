
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

function doSearch(str){

if (str.length>3)
  {
  alert('start search');
 // document.getElementById("refWrapper").innerHTML="";
  return;
  }
 else
  return;
    
//    var url="indexSearch.php";
//    url=url+"?q="+str;
 //   url=url+"&sid="+Math.random();
   // var url="http://localhost:8983/solr/select?";
   // url=url+"&q="+str;
   // url=url+"&fq=&start=0&rows=10&fl=&qt=&wt=&explainOther=&hl.fl=";
    
  //  $.get(url, function(data){ 
   // alert(data);
  // document.getElementById("refWrapper").innerHTML=data; 
 //});
   
}