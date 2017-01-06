<?php

function kassa_form($form,$form_state){	
	
	$form['dateselect_start'] = array(
	'#type' => 'date_select',
	'#date_format' => 'j/n/Y',	
	'#date_label_position' =>'', 
	'#title' => t('Start date'),
	'#default_value' => variable_get('kassa_start',date("now")),
	'#date_year_range' => '-5:+0',
	);
	

	$form['dateselect_end'] = array(
        '#type' => 'date_select',
        '#date_format' => 'j/n/Y',
        '#date_label_position' =>'',
        '#title' => t('End date'),
        '#default_value' => variable_get('kassa_end',date("now")),
        '#date_year_range' => '-5:+0',
        );	
	$form['submit'] = array(
	'#type' => 'submit',
	'#value' => t('Submit'),
	);
	
	 $row['initieel'] = array(
        'id'=> null,
        'time'=> '<b> Initieel </b>',
	'description'=> '',
	'bedrag' =>'',
        );

	$header = array(t('ID'),t('Time'),t('Description'),t('Amount'));	

	foreach(kassa_get_kassa() as $kassa){
		array_push($header,$kassa->kassa);
		$initieel = kassa_initieel(strtotime(variable_get('kassa_start')),$kassa->kassa_id); 
		$row['initieel'][$kassa->kassa_id] = kassa_price_to_euro($initieel);
		$totaal_kassa[$kassa->kassa_id] = $initieel;
		
	}



	$q = db_select('kassa_transactie');
	$q->condition('kassa_transactie.tijdstip',strtotime(variable_get('kassa_start')),'>=')
	->condition('kassa_transactie.tijdstip',strtotime(variable_get('kassa_end')),'<=')
	->fields('kassa_transactie');

	foreach($q->execute()->fetchAll() as $transactie){
		$color = $transactie->bedrag >= 0 ? "green" : "red";	
		if($transactie->type == 'verkoop'){	
			$row[$transactie->transactie_id] = array(
			l('#'.$transactie->transactie_id,'/admin/kassa/add_registration/' . kassa_get_verkoop($transactie->transactie_id)->verkoop_id),
			date('j-n-Y h:i',$transactie->tijdstip),
			$transactie->omschrijving,
			'<font color="'. $color.'">  '. kassa_price_to_euro($transactie->bedrag) . '</font>',
			);	
		}else{
			$row[$transactie->transactie_id] = array(
                        '#'.$transactie->transactie_id,
                        date('j-n-Y h:i',$transactie->tijdstip),
                        $transactie->omschrijving,
                        '<font color="'.$color.'"> '. kassa_price_to_euro($transactie->bedrag) . '</font>',
                        );

		}
		foreach(kassa_get_kassa() as $kassa){
			if($transactie->kassa_id == $kassa->kassa_id){
				$totaal_kassa[$kassa->kassa_id]+= $transactie->bedrag;
			}
				array_push($row[$transactie->transactie_id], kassa_price_to_euro($totaal_kassa[$kassa->kassa_id]));	
		} 
	
	}	
	$form['kassa'] = array(
	'#title' => t('Results'),
	'#type' => 'item',
	'#markup' => theme('table',array('header' => $header,'rows'=>$row)),
	);
	
	return $form;
} //END KASSA FORM


function kassa_initieel($tijdstip_start,$kassa_id){
	$q = db_select('kassa_transactie');
	$q->condition('tijdstip',$tijdstip_start,'<')
	->condition('kassa_id',$kassa_id,'=')
	->fields('kassa_transactie');
	$som = 0;
	$r = $q->execute()->fetchAll();
	foreach($r as $transactie){
		$som += $transactie->bedrag;	
	}	
	return $som;
}

function kassa_form_validate($form,$form_state){
	if(strtotime($form_state['values']['dateselect_start']) >= strtotime($form_state['values']['dateselect_end'])){
		form_set_error('dateselect_start',t('The start date must be before the end date.'));
	}
}
 
function kassa_form_submit($form,$form_state){
	variable_set('kassa_start',$form_state['values']['dateselect_start']);
	variable_set('kassa_end',$form_state['values']['dateselect_end']);	
}
