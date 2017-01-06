<?php

function kassa_verkoop_form($form,$form_state,$verkoop = NULL){
	if(is_null($verkoop)){
		$verkoop = new StdClass();
		$verkoop->persoon_id = null;
		$verkoop->verkoop_id = null;	
		$verkoop->transactie_id = null;
		$form['#verkoop'] = false;
	}else{
		$form['#verkoop'] = $verkoop;
	
	}	
	$form['naam'] = array(
	'#title' => t('Name'),
	'#type' => 'textfield',
	'#size' => 40,
	'#required' => true,
	'#autocomplete_path' => 'ldb/autocomplete/persoon',
	'#default_value' => isset($verkoop->persoon_id) ? ldb_persoon_load($verkoop->persoon_id)->voornaam . ' ' . ldb_persoon_load($verkoop->persoon_id)->achternaam: '',
	'#disabled' => isset($verkoop->persoon_id), 
	);

	$header = array(t('What'), t('Member price'),t('Non member price'),t('Buy?'),t('Cash desk'));
	$form['table'] = array(
	'#type' => 'formtable',
	'#header' => $header,
	'#title' => t('Overview'),
	'#tree' => 'true',
	);
	$row = array();
	foreach(kassa_get_kassa() as $kassa){
	
	   if(is_object($form['#verkoop']) or strcmp(variable_get($kassa->kassa.'_enabled'),'onwaar') != 0){ 
		$row[$kassa->kassa_id] = $kassa->kassa;
	    }
	}
	foreach(kassa_get_ding() as $ding){
		$verkocht_info = kassa_get_verkocht_ding($verkoop->verkoop_id,$ding->ding_id);
	//omslachtige manier voor kassa_id
	if(isset($verkocht_info->verkoop_id)){
		$t = db_select('kassa_transactie_verkoop');
		$t->condition('verkoop_id',$verkocht_info->verkoop_id)
		->fields('kassa_transactie_verkoop');
		$s= $t->execute()->fetch();	
		$q = db_select('kassa_transactie');
		$q->condition('transactie_id',$s->transactie_id)
		->fields('kassa_transactie');
		$r = $q->execute()->fetch();
	}	
	if($ding->zichtbaar){
	$form['table'][$ding->ding_id] = array(
		'name' => array(
			'#type' => 'item', 
			'#markup' => '<p>' . l($ding->naam ,'/admin/kassa/ding/'. $ding->ding_id). '</p>'
			),
		'prijs_leden' => array(
			'#type' => 'item',
			'#markup' => '<p>' . kassa_price_to_euro($ding->prijs_leden) . '</p>'
			),
		'prijs_niet_leden' => array(
			'#type' => 'item', 
			'#markup' => '<p>' .kassa_price_to_euro($ding->prijs_niet_leden) . '</p>'),
		'checkbox' => array(
			'#type' => 'checkbox',
			'#disabled' => isset($verkoop->verkoop_id), 
			'#default_value' => isset($verkocht_info->verkoop_id) ? '1' : '0', 
			),
		'kassa' => array(
			'#type' => 'radios', 
			'#options' => $row, 
			'#default_value' => isset($r->kassa_id) ? $r->kassa_id : $ding->kassa_id, 
			'#disabled' => isset($verkoop->verkoop_id), 
			),
			
		);
	}
	}	

	$form['submit'] = array(
	'#type' => 'submit',
	'#value' => t('Save'),
	'#disabled' => isset($verkoop->verkoop_id),
	);

	return $form;
}

function kassa_verkoop_form_validate($form,$form_state){
	$persoon = ldb_persoon_load_from_autocomplete($form_state['values']['naam']);
	foreach($form_state['values']['table'] as $key => $value){
		 if($value['checkbox'] && kassa_reeds_gekocht($persoon->persoon_id,$key)){
			form_set_error($key,t('An unique item was already bought by this person.'));
		}
	}
}
function kassa_reeds_gekocht($persoon_id,$ding_id){
	$d = db_select('kassa_ding');
	$d->fields('kassa_ding')
	->condition('ding_id',$ding_id)
	->condition('uniek',1);
	
	$q = db_select('kassa_verkoop_uniek');
	$q->condition('persoon_id',$persoon_id)
	->condition('ding_id',$ding_id)
	->fields('kassa_verkoop_uniek');	

	return count($d->execute()->fetchAll()) && count($q->execute()->fetchAll()) >= 1; 
}
function kassa_verkoop_form_submit($form,$form_state){
	switch($form_state['triggering_element']['#parents'][0]){
		case 'submit':

		$persoon = ldb_persoon_load_from_autocomplete($form_state['values']['naam']);	
		$verkoop_id = kassa_get_max('kassa_transactie_verkoop','verkoop_id') +1;
		$transactie_id = kassa_get_max('kassa_transactie','transactie_id') + 1;

		db_insert('kassa_transactie_verkoop')
		->fields(array(
		'persoon_id' => $persoon->persoon_id,
		'verkoop_id' => $verkoop_id,
		'transactie_id' => $transactie_id,
		))->execute();
				
		foreach(kassa_get_kassa() as $kassa){
			$tot_bedrag[$kassa->kassa_id] = 0;
		}
		foreach($form_state['values']['table'] as $key => $value){
                        if($value['checkbox']){
                                $ding = kassa_ding_load($key);
                                db_insert('kassa_verkoop_bevat')
                                ->fields(array(
                                'ding_id' => $key,
                                'verkoop_id' => $verkoop_id,
                                ))->execute();

                                if(kassa_ding_load($key)->uniek){
                                        db_insert('kassa_verkoop_uniek')
                                        ->fields(array(
                                        'ding_id' => $key,
                                        'persoon_id' => $persoon->persoon_id,
                                        ))->execute();
                                }
				$tot_bedrag[$ding->kassa_id] += kassa_is_lid($persoon->persoon_id) ? $ding->prijs_leden: $ding->prijs_niet_leden;
                        }
		}
		foreach(kassa_get_kassa() as $kassa){
			if($tot_bedrag[$kassa->kassa_id] != 0){
				db_insert('kassa_transactie')
				->fields(array(
				'transactie_id' => $transactie_id,
				'type' => 'verkoop',
				'tijdstip' => time(),
				'omschrijving' => 'Verkoop aan ' . $persoon->voornaam . ' ' . $persoon->achternaam,
				'bedrag' => $tot_bedrag[$kassa->kassa_id],
				'kassa_id' => $kassa->kassa_id,
				))->execute();
			}
		}
		dsm('saved');	
		break;
	}//END SWITCH
} //END kassa_verkoop_form
