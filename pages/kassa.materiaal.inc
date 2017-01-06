<?php
function kassa_materiaal_form($form,$form_state){
	$form['pickup'] = array(
	'#title' => t('Pickup'),
	'#type' => 'fieldset',
	'#collapsible' => true,
	);
		
	$form['pickup']['table_pickup'] = array(
	'#ttile' => t('pickup'),
	'#type' => 'formtable',
	'#header' => array(
		t('Name'),
		t('Price'),
		t('Guarantee'),
		t('Total'),
		t('picked up'),
		),
	'#tree' => true,
	);
	
	foreach(kassa_materiaal_get_pickup() as $reservatie){
		if(!kassa_materiaal_form_is_picked_up($reservatie->reservatie_id)){
			$form['pickup']['table_pickup'][$reservatie->reservatie_id] = array(
			'name' => array(
				'#type' => 'item',
				'#markup' => materiaal_get_persoon($reservatie->persoon_id)->voornaam . ' ' . materiaal_get_persoon($reservatie->persoon_id)->achternaam,
				),
			'price' => array(
				'#type' => 'item',
				'#markup' => kassa_price_to_euro(materiaal_get_prijs($reservatie->reservatie_id,$reservatie->persoon_id) - materiaal_get_waarborg($reservatie->reservatie_id)),
				),
			'Guarantee' => array(
				'#type' => 'item',
				'#markup' => kassa_price_to_euro(materiaal_get_waarborg($reservatie->reservatie_id)),
				),
			'totaal' => array(
				'#tpye' => 'item',
				'#markup' => kassa_price_to_euro(materiaal_get_prijs($reservatie->reservatie_id,$reservatie->persoon_id)), 					
			),
			'picked_up' => array(
				'#type' => 'checkbox',
				'#default_value' => 'n',
				),
			);
		}

	}
	$form['pickup']['save_pickup'] = array(
	'#type' => 'submit',
	'#value' => t('Save pickup'),
	);

	$form['return'] = array(
	'#title' => t('Return'),
	'#type' => 'fieldset',
	'#collapsible' => true,
	);
	$form['return']['table_return'] = array(
	'#title' => t('return'),
	'#type' => 'formtable',
	'#header' => array(
		t('Name'),
		t('Guarantee'),
		t('Arrived?'),
		t('Loss guarantee'),
		),
	'#tree' => true,
	);

	foreach(kassa_materiaal_get_return() as $reservatie){
		$r = materiaal_reservatie_load($reservatie->reservatie_id);
			$form['return']['table_return'][$reservatie->reservatie_id] = array(
			'Name' => array(
				'#type' => 'item',
				'#markup' => materiaal_get_persoon($r->persoon_id)->voornaam . ' ' . materiaal_get_persoon($r->persoon_id)->achternaam,
			),
			'waarborg' => array(
				'#type' => 'item',
				'#markup' => kassa_price_to_euro(materiaal_get_waarborg($reservatie->reservatie_id)),
				),
			'arrived' => array(
				'#type' => 'radio',
				'#default_value' => 'n',
				),
			'verlies_waarborg' => array(
				'#type' => 'textfield',
				'#size' => 10,
				'#description' => t('In eurocent'),
				'#default_value' => 0,
				),
			);
		
	}

	$form['return']['save_return'] = array(
	'#type' => 'submit',
	'#value' => t('Save Return'),
	);
	return $form;
}
function kassa_materiaal_pickup_form($form,$form_state){
	return $form;
}
function kassa_materiaal_return_form($form,$form_state){
	return $form;
}
function kassa_materiaal_get_pickup(){
	$q = db_select('materiaal_reservatie');
	$q->leftjoin('materiaal_aanvraag_reservatie','mar','mar.reservatie_id = materiaal_reservatie.reservatie_id');
	$q->condition('tijdstip_afhalen',time() - (24*60*60),'>=')
	->fields('materiaal_reservatie')
	->fields('mar');
	
	return $q->execute()->fetchAll();
}
function kassa_materiaal_get_return(){
	$q = db_select('materiaal_status')
	->fields('materiaal_status')
	->isNull('teruggebracht');
        return $q->execute()->fetchAll();
}
function kassa_materiaal_form_submit($form,$form_state){	
	 switch ($form_state['triggering_element']['#parents'][0]) {
		case 'save_pickup':


			foreach($form_state['values']['table_pickup'] as $key => $value){
				if($value['picked_up'] != 'n'){
				$tijd = time();
				$transactie_id = kassa_get_max('kassa_transactie','transactie_id');
				db_insert('kassa_transactie')
				->fields(array(
				'transactie_id' => $transactie_id + 1,
				'type' => 'verhuur',
				'omschrijving' => 'verhuur #' . l($key,'/materiaal/validate_reservatie/'. $key),
				'tijdstip' => $tijd,
				'bedrag' => materiaal_get_prijs($key,$reservatie->persoon_id),
				'kassa_id' => 2,
				))->execute();
				db_insert('materiaal_status')
				->fields(array(
				'reservatie_id' => $key,
				'afgehaald' => 1,
				'teruggebracht' => null,
				))->execute();			
			  }
			}
		break;
		case 'save_return':
			foreach($form_state['values']['table_return'] as $key => $value){
                                if($value['arrived'] != 'n'){
                                $tijd = time();
				$bedrag = $value['verlies_waarborg'];
				
                                $transactie_id = kassa_get_max('kassa_transactie','transactie_id');
                                db_insert('kassa_transactie')
                                ->fields(array(
                                'transactie_id' => $transactie_id + 1,
                                'type' => 'waarborg',
                                'tijdstip' => $tijd,
				'omschrijving' => 'teruggave waarborg ' .  l($key,'/materiaal/validate_reservatie/'. $key),
				'bedrag' => - (materiaal_get_waarborg($key) - $bedrag),
				'kassa_id' => 2,
                                ))->execute();

				db_update('materiaal_status')
                                ->fields(array( 
                                'teruggebracht' => 1,
                                ))->condition('reservatie_id',$key)->execute();

                         }
                        }

		break;
	}
}
function kassa_materiaal_form_validate($form,$form_state){
	foreach($form_state['values']['table_return'] as $key => $value){
    		if($value['arrived'] != 'n' && $value['verlies_waarborg'] < 0){
                          form_set_error($values['verlies_waarborg'],t('Please enter a positive integer'));   
           	}
      	}

}
function kassa_materiaal_form_is_picked_up($reservatie_id){
	$result = db_select('materiaal_status')
		  ->fields('materiaal_status')
		  ->condition('reservatie_id',$reservatie_id)
		  ->range(0, 1)
		  ->execute()
		  ->rowCount();
	
	if ($result == 0) {
		return false;
	}
	return true;
}
