<?php

function kassa_count_form($form,$form_state){
	$form['#attached']['css'] = array(
 	 drupal_get_path('module', 'kassa') . '/kassa.css',
	);

	$form['kassa_count'] = array(
	'#type' => 'fieldset',
	'#title' => t('Count cash desk'),
	'#collapsible' => true,
	);

	$options = array(10000, 5000,2000,1000,500,200,100,50,20,10,5,2,1);

	foreach($options as $op){
	
		$form['kassa_count'][$op] = array(
		'#type' => 'textfield',
		'#title' => t(kassa_price_to_euro($op).' x '),
		'#size'=> 5,
		'#default_value'=> 0,
		'#prefix' => '<div class="container-inline">', 
		'#suffix' => '</div>',
		);
	} 
	$form['kassa_count']['count'] = array(
	'#type' => 'submit',
	'#value' => t('Count'),	
	);

	$q = db_select('kassa_kassa');
	$q->fields('kassa_kassa');
		
	$form['kassa_telling'] = array(
	'#type' => 'fieldset',
        '#title' => t('diffrence amount cash desk'),
        '#collapsible' => true,
	);
	$opt;
	foreach($q->execute()->fetchAll() as $kassa){	
		$opt[$kassa->kassa_id] = $kassa->kassa;
	}
	$form['kassa_telling']['diff'] = array(
                '#title' => t('diffrence'),
                '#description' => t('In eurocent'),
                '#type' => 'textfield',
                '#size' => 10,
                '#type' => 'textfield',
                '#size' => 10,
                '#default_value'=> 0,
        );
	$form['kassa_telling']['kassa'] = array(
	'#type' => 'select',
	'#options' => $opt,
	'#title' => t('Cash desk'),
	);
	
	$form['kassa_telling']['close'] = array(
	'#type' => 'submit',
        '#value' => t('Save'),
	);

	$form['kassa_transactie'] = array(
	'#type' => 'fieldset',
	'#title' => t('Cash desk transaction'),
	'#collapsible' => true,	
	);

	$form['kassa_transactie']['origin'] = array(
	'#type' => 'select',
	'#options' => $opt, 
	'#title' => t('From'),
	);
	$form['kassa_transactie']['to'] = array(
        '#type' => 'select',
        '#options' => $opt,
        '#title' => t('To'),
        );
	$form['kassa_transactie']['sum'] = array(
        '#type' => 'textfield',
        '#title' => t('sum'),
	'#size' => 10,
        );
	$form['kassa_transactie']['transfer'] = array(
	'#type' => 'submit',
	'#value' => t('Transfer'),
	);
	
	$form['kassa_kringensteun'] = array(
	'#type' => 'fieldset',
	'#title' => t('Count Kringensteun'),
	'#collapsible' => true,
	);
	$form['kassa_kringensteun']['steun'] = array(
	'#type' => 'textfield',
	'#title' => t('Amount'),
	'#size' => 10,
	'#description' => t('In eurocent'),
	);
	$form['kassa_kringensteun']['bereken'] = array(
	'#type' => 'submit',
	'#value' => t('Calculate'),
	);

	return $form;
}
function kassa_count_form_validate($form,$form_state){
	switch($form_state['triggering_element']['#parents']['0']){
	case 'count':
	break;
	case 'close':
	break;
	case 'transfer':
	if($form_state['values']['origin'] == $form_state['values']['to']){
		form_set_error('to',t('You can\'t transfer money between the same cash desk'));
	}	
	break;
	}
}
function kassa_count_form_submit($form,$form_state){
	switch($form_state['triggering_element']['#parents']['0']){
	case 'count':	
		$options = array(10000, 5000,2000,1000,500,200,100,50,20,10,5,2,1);

		$count = 0;
		foreach($options as $op){
			$count+= $op *$form_state['values'][$op]; 
		}
		dsm(kassa_price_to_euro($count));
	break;
	case 'close':
	
	if($form_state['values']['diff'] != 0){
		$transactie_id = kassa_get_max('kassa_transactie','transactie_id') +1;
                $q = db_insert('kassa_transactie')
                ->fields(array(
                'tijdstip' => time(),
                'type' => 'telling',
                'transactie_id' => $transactie_id,
                'omschrijving' => 'telling van ' . kassa_kassa_load($form_state['values']['kassa'])->kassa,
		'bedrag' => $form_state['values']['diff'],
		'kassa_id' => $form_state['values']['kassa'],
                ))->execute();

		dsm('Saved');
	}
	break;
	case 'transfer':
		$transactie_id = kassa_get_max('kassa_transactie','transactie_id') +1;
		db_insert('kassa_transactie')
                ->fields(array(
                'tijdstip' => time(),
                'type' => 'transactie',
		'omschrijving' => 'Transactie vanuit ' . kassa_kassa_load($form_state['values']['origin'])->kassa,  
		'transactie_id' => $transactie_id,
		'kassa_id' => $form_state['values']['origin'],
		'bedrag'=> - $form_state['values']['sum'],
                ))->execute();
		
	
		 db_insert('kassa_transactie')
                ->fields(array(
                'tijdstip' => time(),
                'type' => 'transactie', 
		'omschrijving' => 'Transactie naar ' . kassa_kassa_load($form_state['values']['to'])->kassa,  
                'transactie_id' => $transactie_id+1,
		'bedrag' => $form_state['values']['sum'],
		'kassa_id' => $form_state['values']['to'], 
                ))->execute();

	break;
	case 'bereken':
	foreach(kassa_get_kringen() as $kring){	
		kassa_calculate_kringensteun($kring,$form_state['values']['steun']);
	}
	break;
	}
}
function kassa_calculate_kringensteun($kring,$bedrag){
	$totaal_studenten = 0;
	foreach(kassa_get_kringen() as $k){
		$totaal_studenten += variable_get($k);
	}
	$steun = ($bedrag * 0.05) + ($bedrag * 0.75) * (variable_get($kring) / $totaal_studenten);
	dsm($kring . t(' recieves a total of ') . kassa_price_to_euro($steun));

}
