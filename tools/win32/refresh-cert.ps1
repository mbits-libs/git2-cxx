param(
    [string]$password,
    [string]$filename
)

$params = @{
    Type = 'Custom'
    Container = 'test*'
    Subject = 'CN=Marcin Zdun'
    TextExtension = @(
        '2.5.29.37={text}1.3.6.1.5.5.7.3.3',
        '2.5.29.17={text}email=mzdun@midnightbits.com' )
    KeyUsage = 'DigitalSignature'
    KeyAlgorithm = 'RSA'
    KeyLength = 2048
    NotAfter = (Get-Date).AddMonths(12)
    CertStoreLocation = 'Cert:\CurrentUser\My'
}
$cert = New-SelfSignedCertificate @params

$pwd = ConvertTo-SecureString -String $password -Force -AsPlainText

$file = Export-PfxCertificate -Cert $cert -FilePath $filename -Password $pwd

Get-ChildItem Cert:\CurrentUser\My | Where-Object {$_.Thumbprint -match $cert.Thumbprint} | Remove-Item 
