/* cpf.js */
/*
//this function includes all necessary js files for the application
function include(file)
{

  var script  = document.createElement('script');
  script.src  = file;
  script.type = 'text/javascript';
  script.defer = false;

  document.getElementsByTagName('body').item(0).appendChild(script);

}
include('/js/jquery.js');
include('/js/tablednd.js');
include('/js/jquery.tablesorter.min.js');
include('/js/jquery-ui-1.7.2.custom.min.js');
*/

function connectLists(selector,connect) {
	selector = $('.cpf_draggable > #sortoptions').innerhtml();
	$(selector).sortable('option', 'connectWith', connect);
}

function cpf_init() {
	
	$('.cpf_sortable').tablesorter({
		widgets: ['zebra']
	});
	
	$('.sortinfo,.sortoptions').css('display','none');

	
	$('table.cpf_draggable tbody').attr('id',function(cnt){
			return 'tbody_'+cnt;
		}).each(function() {
			$('#'+this.id).addClass('cpf_draggable');
			//alert($('#'+this.parentNode.id).attr('id'));
			$('#'+this.parentNode.id).removeClass('cpf_draggable');
		});

	$('.cpf_draggable').attr('id',function(cnt) {
		return 'drag_'+cnt;
	}).each(function(){});

	
	$('.cpf_draggable').sortable({
		tolerance: 'pointer',
		cursorAt: 'top,left',
		helper : function(event,element) {
			clone = $('#'+element.attr('id')).clone();
			clone.css("border","1px solid black");
			return clone;
			
		},
		update : function(event,ui) {
			//alert('cpid: '+this.parentNode.id+' old_pid: '+this.id);
			//alert("upd");
			//alert($('#'+this.id).sortable('toArray'));
			//var findstr = "#sort_"+this.id;
			var infoStr = $(".sortinfo",this).html();
			if (infoStr == null) {
				//alert("could be dealing with table...");
				infoStr = $(".sortinfo td",this.parentNode).html();
				//alert(infoStr);
			}
			var orderStr = $('#'+this.id).sortable('toArray').toString();
			$.post("/special/reorder.html",{info:infoStr, order: orderStr},
				function(data){
					//$("#debugArea").html("Order saved! -- " + debugStr);
					//alert("Data Loaded: " + data);
				});
			$("#"+this.id+" tr:odd").removeClass('alt');
			$("#"+this.id+" tr:even").addClass('alt');
		}
	});
	
	$('.cpf_draggable > .sortoptions').each(function() {
		selector = "."+$(this).html();
		//alert(selector);
		$(selector).sortable('option', 'connectWith', selector);
	});
	
}