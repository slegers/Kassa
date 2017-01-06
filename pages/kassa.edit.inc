<?php

function kassa_edit_form($form,$form_state){

	foreach(kassa_get_kassa() as $kassa){	
		$form[$kassa->kassa_id] = array(
		'#type' => 'fieldset',
		'#title' => t('Settings '. $kassa->kassa),
		'#collapsible' => true,
		);
		$form[$kassa->kassa_id][$kassa->kassa_id] = array(
		'#type' => 'select',
		'#title' => t('Enabled'),
		'#options' => drupal_map_assoc(array(t('true'),t('false'))),
		'#default_value' => variable_get($kassa->kassa.'_enabled'),
		'#required' => true,
		);
	}
	$form['submit'] = array(
	'#type' => 'submit',
	'#value' => t('Save'),
	);
	return $form;
}

function kassa_edit_form_submit($form,$form_state){
	foreach(kassa_get_kassa() as $kassa){	
		if(strcmp($form_state['values'][$kassa->kassa_id],'onwaar')){
			variable_set($kassa->kassa .'_enabled','waar');
		}else{
			variable_set($kassa->kassa .'_enabled','onwaar');
		}
	}		
}	
