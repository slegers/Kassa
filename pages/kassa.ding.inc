<?php

function kassa_ding_form($form,$form_state,$ding = null){
	if(is_null($ding)){
		$ding = new StdClass();
		$ding->naam = '';
		$ding->prijs_leden = 0;
		$ding->prijs_niet_leden = 0;
		$ding->uniek = null;
		$ding->verkoopbaar_tot = null;
		$ding->zichtbaar = 1;
		$form['#ding'] = false;
	}else{
		$form['#ding'] = $ding;
	}
	
	$form['name'] = array(
	'#type' => 'textfield',
	'#size' => 30,
	'#title' => t('Name'),
	'#required' => true,
	'#default_value' => isset($ding->naam) ? $ding->naam : '',
	);
	$form['member_price'] = array(
 	'#type' => 'textfield',
	'#size' => 10,
	'#title' => t('Member price'),
	'#description' => t('In eurocent'),
	'#required' => true,
	'#default_value' => isset($ding->prijs_leden) ? $ding->prijs_leden : '',
	'#disabled' => $form['#ding'],
	);
	$form['non_member_price'] = array(
	'#type' => 'textfield',
	'#size' => 10,
	'#title' => t('Non member price'),
	'#description' => t('In eurocent'),
	'#required' => true,
	'#default_value' => isset($ding->prijs_niet_leden) ? $ding->prijs_niet_leden : '',
	'#disabled' => $form['#ding'],
	);
	$form['uniek'] = array(
	'#type' => 'checkbox',
	'#title' => t('Unique?'),
	'#description' => t('Check if a buyer is only allowed to buy one item at member price.'),
	'#default_value' => isset($ding->uniek) ? $ding->uniek : '',
	);

	$form['verkoopbaar_tot'] = array(
	'#type' => 'radios',
	'#title' => t('Salable till'),
	'#description' => t('Cehck if this item\'s price never changes.'),
	'#default_value' => isset($ding->verkoopbaar_tot) ? 2 : 1,
	'#options' => array( 
		1 =>t('Always'),
		2 => t('Choose a date'),
	),
	'#ajax' => array(
		'callback' => 'kassa_ding_date_callback',
		'wrapper' => 'date',
		'effect' => 'slide',
		),
	); 
	$form['date'] = array( 
		'#prefix' => '<div id="date">',
		'#suffix' => '</div>',
		'#type' => 'item',
		'#display'=>true,
	);

if((!empty($form_state['values']['verkoopbaar_tot']) && $form_state['values']['verkoopbaar_tot'] == '2')or $form['#ding']){

	$form['date']['date_select'] = array(
	'#type' => 'date_select',
	'#title' => t('Last day of Sale'),
	'#date_format' => 'j/n/Y',
	'#default_value' => variable_get('kassa_start',date("now")),
	);	
	$form['zichtbaar'] = array(
	'#type' => 'checkbox',
	'#default_value' => isset($ding->zichtbaar) ? $ding->zichtbaar : 0,
	'#title' => t('Visible?'),
	);	
}
	$row = array();
	foreach(kassa_get_kassa() as $kassa){
	  $row[$kassa->kassa_id] = $kassa->kassa;
	}
	$form['cash_desk'] = array(
	'#type' => 'radios',
	'#options' => $row,
	'#title' => t('Cash desks'),
	'#required' => true,
	'#default_value' => isset($ding->kassa_id) ? $ding->kassa_id : 1,
	);
	$form['submit'] = array(
	'#type' => 'submit',
	'#value' => t('Save'),
	); 
	return $form; 
}
function kassa_ding_date_callback($form,$form_state){
	return $form['date'];
}

function kassa_ding_form_validate($form,$form_state){
	if(!is_numeric($form_state['values']['non_member_price'])){
		form_set_error('non_member_price',t('Please enter an positive integer!'));
	}elseif(!is_numeric($form_state['values']['member_price'])){
		form_set_error('member_price',t('Please enter a positive integer!'));
	}
}

function kassa_ding_form_submit($form,$form_state){	
	switch($form_state['triggering_element']['#parents']['0']){
		case 'submit':
		$verkoopbaar_tot = null;
		if($form_state['values']['verkoopbaar_tot'] == 2){
			$verkoopbaar_tot = strtotime($form_state['values']['date_select']); 
		}
		if(!$form['#ding']){	
			db_insert('kassa_ding')
			->fields(array(
			'ding_id' => kassa_get_max('kassa_ding','ding_id') +1,
			'naam' => $form_state['values']['name'],
			'prijs_leden' => $form_state['values']['member_price'],
			'prijs_niet_leden' => $form_state['values']['non_member_price'],
			'uniek' => $form_state['values']['uniek'],
			'verkoopbaar_tot' => $verkoopbaar_tot,
			'kassa_id' => $form_state['values']['cash_desk'],
			'zichtbaar' => $form['#ding'] ? $form_state['values']['zichtbaar'] : 1,
			))->execute();
		}else{
			db_update('kassa_ding')
			->fields(array(
			'naam' => $form_state['values']['name'],
			'uniek' => $form_state['values']['uniek'],
			'verkoopbaar_tot' => $verkoopbaar_tot,
			'zichtbaar' => $form_state['values']['zichtbaar'],
			'kassa_id' => $form_state['values']['cash_desk'],
			))->condition('ding_id',$form['#ding']->ding_id)->execute();
		}
		dsm(t('Saved'));
		break;
	}
}
