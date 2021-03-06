<%@ page contentType="text/html;charset=UTF-8" language="java"%>
<%@ page import="java.util.Date"%>
<%@ page import="java.util.List"%>
<%@ page import="com.google.appengine.api.datastore.*"%>
<%@ page import="com.google.appengine.api.datastore.Query.*"%>
<%@ page import="com.google.appengine.api.datastore.Query.Filter"%>

<%@ page import = "java.io.*,java.util.*" %>

<html>
<head>
<title>Grafica Luminosidad</title>
</head>
<body>
	<script src="https://code.jquery.com/jquery-3.1.1.min.js"></script>
	<script src="https://code.highcharts.com/highcharts.js"></script>
	<script src="https://code.highcharts.com/modules/exporting.js"></script>
	
	<%
	response.setIntHeader("Refresh", 10);
	%>
	

	<%
   	DatastoreService datastore = DatastoreServiceFactory.getDatastoreService();
	%>

	<div id="container"
		style="min-width: 310px; height: 400px; margin: 0 auto"></div>

	<script type="text/javascript">

			var michart;

			function inserta(tiempo,medicion)
			{    
			      michart.series[0].addPoint([tiempo,medicion]);
			}

			
	$(document).ready(function () {
    Highcharts.setOptions({
        global: {
            useUTC: false
        }
    });

    michart=Highcharts.chart('container', {
        chart: {
            type: 'spline',
            animation: Highcharts.svg, // don't animate in old IE
            marginRight: 10,
        },
        title: {
            text: 'Luminosidad habitacion'
        },
        xAxis: {
            type: 'datetime',
            tickPixelInterval: 150
        },
        yAxis: {
            title: {
                text: 'Valor luminosidad'
            },
            min:0,
            max:1024,
            plotLines: [{
                value: 0,
                width: 1,
                color: '#808080'
            }]
        },
        legend: {
            enabled: false
        },
        exporting: {
            enabled: false
        },
        series: [{
            name: 'Datos',       
        }]
    });


 	<%
   	
	//Use class Query to assemble a query
	Filter propertyFilter =new FilterPredicate("tipo", FilterOperator.EQUAL, "luminosidad");
        
	Query q = new Query("Dato-final")
					.addSort("cuando", SortDirection.DESCENDING)
					.setFilter(propertyFilter);
    	
	PreparedQuery pq = datastore.prepare(q);

	List<Entity> l = pq.asList(FetchOptions.Builder.withLimit(20));

	for (Entity result : l) {		
   		Date x=(Date)result.getProperty("cuando");
		Long y = (Long)result.getProperty("valor");
		
        out.println("inserta("+x.getTime()+","+y+");");             
		}
	
      %>
      

}
);	/* fin 	$(document).ready(function () { */

		</script>

</body>
</html>
