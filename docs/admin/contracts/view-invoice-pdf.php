<?
require('fpdf.h');
eval(read_whole_file('view-invoice-common.data'));

//check out http://www.fpdf.org/ for docs

class InvoicePDF extends FPDF {

  function InvoicePDF() {
    $this->FPDF();
  }

  //Page header
  function Header() {

    //Logo
    if (file_exists($_SERVER['DOCUMENT_ROOT'].'/logo.png')) {
      $this->Image($_SERVER['DOCUMENT_ROOT'].'/logo.png',10,8,33);
      $this->Ln(10);
    }

    $this->SetFont('Arial','b',14);
    $this->MultiCell(0,5,"INVOICE",0,'C');
    $this->Ln(4);

    //<hr>
    $this->Cell(180,1,'',1,1,'L',1);
    $this->Ln(5);
  }

  //Page footer
  function Footer()
  {
    //Position at 1.5 cm from bottom
    $this->SetY(-15);
    //Times italic 8
    $this->SetFont('Times','i',8);
    //Page number
    //$this->Cell(0,10,'Page '.$this->PageNo().'/{nb}',0,0,'C');

    $this->Cell(0,10,"Thank you for doing business with us!",0,0,'C');
  }

  function render($ch) {
    $this->SetFont('Arial','',10);
    $this->Cell(30,4,date("Y/m/d", $ch['create_date']),0,0,'L');
    $this->Cell(100,4,$ch['name'],0,0,'L');

    if ($ch['void']) {
      $val = sprintf("<strike>%.02f</strike>", $ch['amount']/100.0);
    } else {
      $val = sprintf("%.02f", $ch['amount']/100.0);
      $rc = $ch['amount'];
    }
    $this->Cell(0,4,$val,0,1,'R');

    return $rc;
  }
}


//Instantiation of inherited class
$pdf=new InvoicePDF();
$pdf->AliasNbPages();
$pdf->AddPage();

if ($invoice['void']) {
  $pdf->SetFont('Arial','bi',10);
  $pdf->Write(5,"This contract is void, and is not payable.");
  $pdf->Ln(5);
} else {

  //draw the two columns interleaved: "from" and "invoice details"
    $pdf->SetFont('Arial','b',10);
    $pdf->Cell(145,0,$u1['details']['company']);
    $pdf->SetFont('Arial','',10);
    $pdf->Cell(0,0,sprintf("invoice #%d-%d", $invoice['contract_id'], $invoice['invoice_id']));
    $pdf->Ln(4);

    $pdf->Cell(145,0,$u1['name']);
    $pdf->Cell(0,0,"issued: ".date('Y-m-d', $invoice['create_date']));
    $pdf->Ln(4);

    $pdf->Cell(145,0,$u1['email']);
    $pdf->Cell(0,0,sprintf("due: %s",$invoice['due_date']));
    $pdf->Ln(4);

    $pdf->MultiCell(145,4,$u1['details']['address']."\n".$u1['details']['address_2']);

    // recipient
    $pdf->Ln(10);
    $pdf->SetFont('Arial','b',10);
    $pdf->Cell(0,0,"Bill to:");
    $pdf->SetFont('Arial','',10);
    $pdf->Ln(2);
    $pdf->MultiCell(0,4,sprintf("%s\n%s\n%s\n%s\n%s", $u2['name'], $u2['details']['company'], $u2['email'], $u2['details']['address'], $u2['details']['address_2']));

    $pdf->Ln(10);
    //table of charges
    $pdf->SetFillColor(100);
    $pdf->SetTextColor(255);
    $pdf->Cell(30,4,"Date",0,0,'L',1);
    $pdf->Cell(100,4,"Name",0,0,'L',1);
    $pdf->Cell(0,4,"Amount",0,1,'R',1);
    $pdf->SetTextColor(0);

	foreach ($invoice_items as $ch) {
       if (substr($ch['name'],0,4) == 'tax ')  { $tax_items[] = $ch; continue; }
       $running += $pdf->render($ch);
    }
    $pdf->Ln(10);

    $pdf->SetFillColor(100);
    $pdf->SetTextColor(255);
    $pdf->SetX(30);
    $pdf->Cell(100,4,"subtotal",0,0,'R',1);
    $pdf->Cell(0,4,sprintf("%.02f", $running/100.0),0,1,'R',1);
    $pdf->SetTextColor(0);

    foreach ($tax_items as $ch) {
        $running += $pdf->render($ch);
    }

    if ($payments>0) {
      $pdf->SetFillColor(100);
      $pdf->SetTextColor(255);
      $pdf->SetX(30);
      $pdf->Cell(100,4,"total payments",0,0,'R',1);
      $pdf->Cell(0,4,sprintf("%.02f", $payments/100.0));
      $pdf->SetTextColor(0);
    }

    $pdf->SetFillColor(100);
    $pdf->SetTextColor(255);
    $pdf->SetX(30);
    $pdf->Cell(100,4,"amount due",0,0,'R',1);
    $pdf->Cell(0,4,sprintf("%.02f", ($running-$payments)/100.0),0,1,'R',1);
    $pdf->SetTextColor(0);
 }

$pdf->Output();
?>
